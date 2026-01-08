#include <iostream>
#include <memory>
#include <string>
#include <cstring>

// Add includes for transport implementations
#include "../client_code/client.h"
#include "../client_code/TCPClientTransport.h"
#include "../client_code/SCTPClientTransport.h"
// When UDP implementation is ready, uncomment the line below:
// #include "../client_code/UDPClientTransport.h"

using namespace std;

int main(int argc, char* argv[]) {

    // Validation of arguments
    // Expecting: ./clientSide [IP] [PROTOCOL]
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " [server_ip] [protocol: tcp|sctp|udp]\n";
        return 1;
    }

    string server_ip = argv[1];
    string proto = argv[2];
    uint16_t port = 575;

    // Pointers to Transport backend
    unique_ptr<Transport> transport_backend;

    // Factory-like selection of transport based on protocol
    if (proto == "tcp") {
        cout << "[Main] Chosen protocol TCP.\n";
        transport_backend = make_unique<TCPClientTransport>();
    } 
    else if (proto == "sctp") {
        cout << "[Main] Chosen protocol SCTP.\n";
        transport_backend = make_unique<SCTPClientTransport>();
    }
    // else if (proto == "udp") {
    //     transport_backend = make_unique<UDPClientTransport>();
    // }
    else {
        cerr << "[Error] Unknown protocol: " << proto << "\n";
        return 1;
    }

    // Create Client with selected transport backend
    // Client doesnt care about the actual transport implementation
	Client theClient(move(transport_backend));

    if (theClient.initialize_transmission(server_ip, port)) {
        cout << "[Main] Connected to " << server_ip << ":" << port << ".\n";
        while(theClient.start_transmission()){
	//theClient.start_transmission();
       /*	if(theClient.get_transport()){
	 	Telemetry stats = transport_backend->get_stats();
        	cout << "Measured RTT: " << stats.rtt_ms << " ms\n";
    	}else{
		cout << "something idk\n";
	}
    } else {
        cerr << "[Main] Connection error.\n";
        return 1;*/
	Transport* active_transport = theClient.get_transport();
	if(active_transport){		//if the pointer is not NULL
		Telemetry stats = active_transport->get_stats();
		cout << "Measured RTT: " << stats.rtt_ms << " ms\n";
		cout << "Measured jitter: " << stats.jitter << " ms\n";
	}
    }	//while(1)
    }

    return 0;
}
