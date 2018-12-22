#include <modbus/modbus.h>
#include "Bus.h"
#include <signal.h>
#include <iostream>
using namespace std;

Bus* b;

void my_handler(int s) {
	cout << "Stopping" << endl;
	exit(1);
}

int main() {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	while (true) {
		try {
			b = new Bus("bus.json");
			(*b).sendData();
		} catch (...) {
			(*b).~Bus();
			delete(b);
		}
	}
	return 0;
}
