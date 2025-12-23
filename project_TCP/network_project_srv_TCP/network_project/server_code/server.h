#ifndef SERVER_H
#define SERVER_H

#include "TCPServerTransport.h"
#include <memory>
#include <vector>
#include <string>
using namespace std;

class Server{
private:
	unique_ptr<TCPServerTransport> transport;
	bool is_running;
	void handle_client_session(unique_ptr<Transport> cli_conn);
public:
	Server(unique_ptr<TCPServerTransport> st);
	~Server();

	bool initialize_transmission(const string& addr, uint16_t port);
        void start_transmission();
        void close_transmission();


};

#endif
