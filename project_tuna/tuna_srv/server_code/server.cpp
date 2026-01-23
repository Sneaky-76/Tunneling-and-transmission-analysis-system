#include "server.h"		//if any of those headers change its residing place
				//the change should be properly applied in includes
#include "../core/ServerTransport.h"
#include <iostream>
#include <thread>
#include <syslog.h>
#include "UDPServerTransport.h"
#include <arpa/inet.h>

Server::Server(unique_ptr<ServerTransport> st) : transport(move(st)), is_running(false) {
	openlog("NetworkAnalyzerServ", LOG_PID, LOG_USER);
}

Server::~Server(){
	close_transmission();
	closelog();
}

bool Server::initialize_transmission(const string& addr, uint16_t port){
	if(transport->bindAndListen(port)){
		syslog(LOG_INFO, "Server bound to port %u resulted in a success", port); //%u for uint
		is_running = true;
		return true;
	}
	syslog(LOG_ERR, "Server failed to bound to port %u", port);
	return false;
}

void Server::handle_client_session(unique_ptr<Transport> cli_conn){
	//OODB auth here probably
	while(1){
	syslog(LOG_INFO,"New client connected. Authentication . . .");
	vector<uint8_t> buff;
	
	ssize_t bytes = cli_conn->recieve(buff);

	if(bytes > 0){
		string clientMsg(buff.begin(), buff.end());
		cout << "[Server] Recieved: " << clientMsg << endl;

		string response = "ACK: message has been recieved by the server.";
		//cout << "Sending a response: " <<  response << endl;
		vector<uint8_t> responseData(response.begin(), response.end());

		cli_conn->send(responseData);
		syslog(LOG_DEBUG, "Recieved %zd bytes.", bytes);		//%z for ssize_t, %d for decimal
		//cli_conn->send(buff);	//echoing data back
	}
	}//while(1)
	cli_conn->close_connection();
}

void Server::start_transmission(){
	// Initialization of the background traffic sink on port 9999
    bg_sink.start(9999);

	cout << "Server running and awaiting for connection . . .\n";

	if (auto* udp = dynamic_cast<UDPServerTransport*>(transport.get())) {

        vector<uint8_t> buffer;
        sockaddr_in from{};

        while (is_running) {
            ssize_t n = udp->receiveFrom(buffer, from);
            if (n <= 0) continue;

            cout << "[UDP] Packet from "
                 << inet_ntoa(from.sin_addr)
                 << ":" << ntohs(from.sin_port)
                 << " size=" << n << endl;

            udp->sendTo(buffer, from); 
        }
        return;
    }
	
	while(is_running){
		unique_ptr<Transport> client = transport->acceptClient();

		if(client){
			//thread for simultaneous client handlig
			thread(&Server::handle_client_session, this, move(client)).detach();
		}
	}
}

void Server::close_transmission(){

	is_running = false;
	if(transport){
		transport->close_connection();
	}
}
