/*
 * ZabbixSender.cpp
 *
 *  Created on: Dec 14, 2018
 *      Author: nikita
 */

#include "ZabbixSender.h"
#include <json-c/json.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
using namespace std;

ZabbixSender::ZabbixSender(char* ip, int port) {
	this->ip = ip;
	this->port = port;
}

ZabbixSender::ZabbixSender(char* filename) {
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
	struct json_object *obj, *record;
	obj = json_tokener_parse(buffer);

	json_object_object_get_ex(obj, "IP", &record);
	ip = (char*) json_object_get_string(record);
	json_object_object_get_ex(obj, "port", &record);
	port = json_object_get_int(record);
	//cout << "IP: " << ip << "\nport: " << port << endl;
}

int ZabbixSender::createSocket(char* ip, int port) {
	struct sockaddr_in address;
	struct sockaddr_in serv_addr;
	int s = 0;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(s, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	return s;
}

void ZabbixSender::sendSingleValue(char* host, char* key, char* value) {
	struct json_object *sendJson, *dataArray, *dataRecord;
	char* sendJsonString;
	int length;
	sendJson = json_object_new_object();
	dataArray = json_object_new_array();
	dataRecord = json_object_new_object();
	json_object_object_add(dataRecord, "host", json_object_new_string(host));
	json_object_object_add(dataRecord, "key", json_object_new_string(key));
	json_object_object_add(dataRecord, "value", json_object_new_string(value));
	json_object_array_add(dataArray, json_object_get(dataRecord));
	json_object_object_add(sendJson, "request",
			json_object_new_string("sender data"));
	json_object_object_add(sendJson, "data", dataArray);

	sendJsonString = (char*)json_object_to_json_string(sendJson);
	length = strlen(sendJsonString);
	string package("ZBXD\x01");
	package += (length & 0x000000FF);
	package += (length >> 8) & 0x000000FF;
	package += (length >> 16) & 0x000000FF;
	package += (length >> 24) & 0x000000FF;
	package += '\x00';
	package += '\x00';
	package += '\x00';
	package += '\x00';
	package += sendJsonString;
	//struct sockaddr_in serv_addr;

	int sock = createSocket(ip, port);

	send(sock , package.c_str(), length + 13 , 0 );
	char buffer[1024] = {0};
	read(sock , buffer, 1024);
	char cutBuffer[1011] = {0};
	for (int i =13; i < 1024; i++){
		cutBuffer[i - 13] = buffer[i];
	}
	//cout << cutBuffer << endl;
	close(sock);
}

ZabbixSender::~ZabbixSender() {
}

