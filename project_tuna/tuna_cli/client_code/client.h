#ifndef CLIENT_H
#define CLIENT_H

#include "../core/transport.h"	//first common folder
#include "TrafficGenerator.h"
#include <vector>
#include <memory>		//where unique_ptr resides
#include <string>
#include <ifaddrs.h>

using namespace std;

class Client{

private:
	unique_ptr<Transport> transport;
	TrafficGenerator bg_traffic;

	string server_ip;
public:
	Client(unique_ptr<Transport> transport);
	
	Transport* get_transport() const {return transport.get();}	
	bool initialize_transmission(const string& addr, uint16_t port);
	bool start_transmission();
	void close_transmission();
	
	string find_active_interface(); //for proper real packet loss simulation implementation
};

#endif
