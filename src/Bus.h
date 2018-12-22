/*
 * Bus.h
 *
 *  Created on: Dec 14, 2018
 *      Author: root
 */

#include <modbus/modbus.h>
#include "Device.h"
#include "ZabbixSender.h"
#ifndef BUS_H_
#define BUS_H_

typedef struct {
    int id;
    int period;
} usedMetric;

class Bus {
public:
	Bus(char* filename);
	void printInfo();
	void sendData(long duration);
	void sendData();
	virtual ~Bus();
private:
	ZabbixSender *zs;
	modbus_t *mb;
	Device *devices;
	int devicesNum;
	long timeMillis();
};

#endif /* BUS_H_ */
