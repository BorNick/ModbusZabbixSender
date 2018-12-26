/*
 * ZabbixSender.h
 *
 *  Created on: Dec 14, 2018
 *      Author: nikita
 */

#ifndef ZABBIXSENDER_H_
#define ZABBIXSENDER_H_
#include <arpa/inet.h>

class ZabbixSender {
public:
	ZabbixSender(char* ip, int port);
	ZabbixSender(char* filename);
	void sendSingleValue(char* host, char* key, char* value);
	virtual ~ZabbixSender();
private:
	char* ip;
	int port;
	int createSocket(char* ip, int port);
};

#endif /* ZABBIXSENDER_H_ */
