#ifndef SERVER_H
#define SERVER_H

#include "../core/ServerTransport.h"
#include "../core/DatabaseManager.h"
#include "TrafficSink.h"
#include <memory>
#include <vector>
#include <string>
using namespace std;

class Server{
private:
	unique_ptr<ServerTransport> transport;
    TrafficSink bg_sink; // Added instance of TrafficSink
	DatabaseManager db_manager; // Added instance of DatabaseManager
	bool is_running;
	void handle_client_session(unique_ptr<Transport> cli_conn);
public:
	Server(unique_ptr<ServerTransport> st);
	~Server();

	bool initialize_transmission(const string& addr, uint16_t port);
        void start_transmission();
        void close_transmission();


};

#endif
