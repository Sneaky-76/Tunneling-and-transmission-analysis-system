#ifndef CLIENT_H
#define CLIENT_H

#include "../core/transport.h"	//first common folder
#include <vector>
#include <memory>		//where unique_ptr resides
using namespace std;

class Client{

private:
	unique_ptr<Transport> transport;
public:
	Client(unique_ptr<Transport> transport);
	
	bool initialize_transmission(const string& addr, uint16_t port);
	void start_transmission();
	void close_transmission();
};

#endif
