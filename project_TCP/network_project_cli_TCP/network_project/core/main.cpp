#include "../client_code/TCPClientTransport.h"
#include "../client_code/client.h"
#include <iostream>
#include <memory>

using namespace std;

int main(int argc, char* argv[]){

	//was the input correct check (here user provided less than 2 arguments)
	if(argc < 2){
		cerr << "The proper input is: " << argv[0] << " [server ip]\n";
		return 1;
	}

	string server_ip = argv[1];
	uint16_t port = 575;

	//later choosing the protocol in here
	auto tcp_backend = make_unique<TCPClientTransport>();

	Client theClient(move(tcp_backend));

	if(theClient.initialize_transmission(server_ip, port)) {
		cout << "Connected to " << server_ip << " on port " << port << ".\n";
	theClient.start_transmission();
	} else {
		cerr << "Connecting has failed.\n";
		return 1;
	}
	return 0;
}
