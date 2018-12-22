/*
 * Device.cpp
 *
 *  Created on: Dec 14, 2018
 *      Author: root
 */

#include "Device.h"
#include <json-c/json.h>
#include <iostream>
using namespace std;

Device::Device(char* filename) {
	slaveId = 1;
	char * buffer = 0;
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
	struct json_object *obj, *metricsArray, *curMetric, *record;
	obj = json_tokener_parse(buffer);

	json_object_object_get_ex(obj, "metrics", &metricsArray);
	metricsNum = json_object_array_length(metricsArray);
	metrics =  new metric[metricsNum];
	for(int i = 0; i < metricsNum; i++){
		curMetric = json_object_array_get_idx (metricsArray, i);
		json_object_object_get_ex(curMetric, "name", &record);
		metrics[i].name = (char*) json_object_get_string(record);
		json_object_object_get_ex(curMetric, "register", &record);
		metrics[i].reg = json_object_get_int(record);
		json_object_object_get_ex(curMetric, "type", &record);
		metrics[i].type = (char*) json_object_get_string(record);
		json_object_object_get_ex(curMetric, "multiplier", &record);
		metrics[i].multiplier = json_object_get_double(record);
		metrics[i].used = false;
		metrics[i].prevSubmit = 0;
	}
}

void Device::setSlaveId(int n){
	slaveId = n;
}

int Device::getSlaveId(){
	return slaveId;
}

int Device::getMetricsNum(){
	return metricsNum;
}

Device::~Device() {
	delete(metrics);
}

