/*
 * Bus.cpp
 *
 *  Created on: Dec 14, 2018
 *      Author: root
 */

#include "Bus.h"
#include "Device.h"
#include "ZabbixSender.h"
#include <modbus/modbus.h>
#include <json-c/json.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <iomanip> // setprecision
#include <iostream>
using namespace std;

//Bus::Bus(char* serialPort, int baudRate, char parity, int dataBits,
//		int stopBits) {
//	mb = modbus_new_rtu(serialPort, baudRate, parity, dataBits, stopBits);
//	modbus_connect(mb);
//}

Bus::Bus(char* filename) {
	zs = new ZabbixSender("zabbix.json");
	char * buffer = 0, *serialPort;
	int baudRate, dataBits, stopBits;
	char parity;
	long length;
	FILE * f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = new char[length];
		if (buffer) {
			fread(buffer, 1, length, f);
		}
		fclose(f);
	}
	struct json_object *obj, *record, *devicesArray, *curDevice, *usedMetricsArray, *curUsedMetric;
	obj = json_tokener_parse(buffer);

	json_object_object_get_ex(obj, "serial port", &record);
	serialPort = (char*) json_object_get_string(record);
	json_object_object_get_ex(obj, "baud rate", &record);
	baudRate = json_object_get_int(record);
	json_object_object_get_ex(obj, "parity", &record);
	parity = (json_object_get_string(record))[0];
	json_object_object_get_ex(obj, "data bits", &record);
	dataBits = json_object_get_int(record);
	json_object_object_get_ex(obj, "stop bits", &record);
	stopBits = json_object_get_int(record);

	json_object_object_get_ex(obj, "devices", &devicesArray);
	devicesNum = json_object_array_length(devicesArray);
	devices = (Device*) (::operator new[](devicesNum * sizeof(Device)));
	for(int i = 0; i < devicesNum; i++){
		curDevice = json_object_array_get_idx(devicesArray, i);
		json_object_object_get_ex(curDevice, "filename", &record);
		devices[i] = Device((char*) json_object_get_string(record));
		json_object_object_get_ex(curDevice, "slave id", &record);
		devices[i].setSlaveId(json_object_get_int(record));
		json_object_object_get_ex(curDevice, "used metrics", &usedMetricsArray);
		int usedMetricsNum = json_object_array_length(usedMetricsArray);
		for(int j = 0; j < usedMetricsNum; j++){
			curUsedMetric = json_object_array_get_idx(usedMetricsArray, j);
			json_object_object_get_ex(curUsedMetric, "id", &record);
			int usedMetricId = json_object_get_int(record);
			devices[i].metrics[usedMetricId].used = true;
			json_object_object_get_ex(curUsedMetric, "key", &record);
			devices[i].metrics[usedMetricId].key = (char*) json_object_get_string(record);
			json_object_object_get_ex(curUsedMetric, "period", &record);
			devices[i].metrics[usedMetricId].period = json_object_get_int(record);
		}
	}


	mb = modbus_new_rtu(serialPort, baudRate, parity, dataBits, stopBits);
	modbus_connect(mb);
}

void Bus::printInfo(){
	for(int i = 0; i < devicesNum; i++){
		Device curDevice = devices[i];
		cout << "SlaveId: " << curDevice.getSlaveId() << "\nMetrics:\n";
		for(int j = 0; j < curDevice.getMetricsNum(); j++){
			metric curMetric = curDevice.metrics[j];
			cout << "  id: " << curMetric.id << endl;
			cout << "  name: " << curMetric.name << endl;
			cout << "  register: " << curMetric.reg << endl;
			cout << "  multiplier: " << curMetric.multiplier << endl;
			cout << "  used: " << curMetric.used << endl;
			cout << "  period: " << curMetric.period << endl;
		}
	}
}

void Bus::sendData(long duration){
	cout << "Starting to send data\n";
	long finish = timeMillis() + duration * 1000;
	//std::this_thread::sleep_for (std::chrono::milliseconds(100));
	while(timeMillis() < finish){
		for(int i = 0; i < devicesNum; i++){
			modbus_set_slave(mb, devices[i].getSlaveId());
			for(int j = 0; j < devices[i].getMetricsNum(); j++){
				if(devices[i].metrics[j].used){
					if((timeMillis() - devices[i].metrics[j].prevSubmit) >= devices[i].metrics[j].period){
						double value;
						if(strcmp(devices[i].metrics[j].type, "coil") == 0 || strcmp(devices[i].metrics[j].type, "discrete_input") == 0){
							//cout << devices[i].metrics[j].type << endl;
							uint8_t bit = 0;
							modbus_read_bits(mb, devices[i].metrics[j].reg, 1, &bit);
							//cout << "\nbit: " << (double)bit << "\n\n";
							value = ((double)bit) * devices[i].metrics[j].multiplier;
						} else{
							//cout << devices[i].metrics[j].type << endl;
							uint16_t reg = 0;
							modbus_read_registers(mb, devices[i].metrics[j].reg, 1, &reg);
							value = ((double)reg) * devices[i].metrics[j].multiplier;
						}
						stringstream stream;
						stream << fixed << setprecision(2) << value;
						char* valueAsString = (char*) stream.str().c_str();
						cout << devices[i].metrics[j].key << ": " << valueAsString << endl;
						devices[i].metrics[j].prevSubmit = timeMillis();
						zs->sendSingleValue(devices[i].metrics[j].key, valueAsString);
					}
				}
			}
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(1));
	}
}

void Bus::sendData(){
	cout << "Starting to send data\n";
	while(true){
		for(int i = 0; i < devicesNum; i++){
			modbus_set_slave(mb, devices[i].getSlaveId());
			for(int j = 0; j < devices[i].getMetricsNum(); j++){
				if(devices[i].metrics[j].used){
					if((timeMillis() - devices[i].metrics[j].prevSubmit) >= devices[i].metrics[j].period){
						double value;
						if(strcmp(devices[i].metrics[j].type, "coil") == 0 || strcmp(devices[i].metrics[j].type, "discrete_input") == 0){
							//cout << devices[i].metrics[j].type << endl;
							uint8_t bit = 0;
							modbus_read_bits(mb, devices[i].metrics[j].reg, 1, &bit);
							//cout << "\nbit: " << (double)bit << "\n\n";
							value = ((double)bit) * devices[i].metrics[j].multiplier;
						} else{
							//cout << devices[i].metrics[j].type << endl;
							uint16_t reg = 0;
							modbus_read_registers(mb, devices[i].metrics[j].reg, 1, &reg);
							value = ((double)reg) * devices[i].metrics[j].multiplier;
						}
						stringstream stream;
						stream << fixed << setprecision(2) << value;
						char* valueAsString = (char*) stream.str().c_str();
						cout << devices[i].metrics[j].key << ": " << valueAsString << endl;
						devices[i].metrics[j].prevSubmit = timeMillis();
						zs->sendSingleValue(devices[i].metrics[j].key, valueAsString);
					}
				}
			}
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(1));
	}
}

long Bus::timeMillis(){
	using namespace std::chrono;
	auto now = system_clock::now();
	auto now_ms = time_point_cast<milliseconds>(now);
	auto value = now_ms.time_since_epoch();
	return value.count();
}

Bus::~Bus() {
	delete devices;
	delete zs;
	modbus_close(mb);
	modbus_free(mb);
}

