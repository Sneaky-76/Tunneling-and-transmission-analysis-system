#ifndef TCPSERVER_TRANSPORT_H
#define TCPSERVER_TRANSPORT_H

#include <memory>
#include "../core/transport.h"
using namespace std;

class TCPServerTransport : public Transport{
private:
	int listenfd;
public:
	TCPServerTransport();
	~TCPServerTransport();


	bool bindAndListen(uint16_t port);
	unique_ptr<Transport> acceptClient();
       	ssize_t send(const vector<uint8_t>& data) override;
       	ssize_t recieve(vector<uint8_t>& data) override;
        void close_connection() override;

	//sadly need to add it in here, otherwise the compiler considers incompletion
	bool connectTo(const string& addr, uint16_t port);
};


#endif

