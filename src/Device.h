/*
 * Device.h
 *
 *  Created on: Dec 14, 2018
 *      Author: root
 */

#ifndef DEVICE_H_
#define DEVICE_H_

typedef struct {
    int id;
    char* name;
    int reg;
    char* type;
    double multiplier;
    bool used;
    char* key;
    int period;
    long prevSubmit;
} metric;

class Device {
public:
	Device(char* filename);
	void setSlaveId(int);
	int getSlaveId();
	char* host;
	metric * metrics;
	int getMetricsNum();
	virtual ~Device();
private:
	int slaveId;
	int metricsNum;
};

#endif /* DEVICE_H_ */
