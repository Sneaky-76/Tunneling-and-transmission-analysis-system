#include "../server_code/TCPServerTransport.h"
#include "../server_code/server.h"
#include <iostream>
#include <memory>

using namespace std;

int main(){

	//later choosing the protocol in here
	auto tcp_backend = make_unique<TCPServerTransport>();

	Server theServer(move(tcp_backend));
	uint16_t port = 575;

	if(theServer.initialize_transmission("", port)) {
		cout << "Server listening on port " << port << ".\n";
	theServer.start_transmission();
	} else {
		cerr << "Server has failed to start.\n";
		return 1;
	}
	return 0;
}
