#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <iomanip>            //fixed & setprecision
#include <cstdlib>            //for ::system to run commands in terminal

// Add includes for transport implementations
#include "../client_code/client.h"
#include "../client_code/TCPClientTransport.h"
#include "../client_code/SCTPClientTransport.h"
#include "../client_code/UDPClientTransport.h"

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
    else if (proto == "udp") {
        transport_backend = make_unique<UDPClientTransport>();
    }
    else {
        cerr << "[Error] Unknown protocol: " << proto << "\n";
        return 1;
    }
    /*                                    //VIP deterioration of network conditions implementation via terminal (to do)
    int interface_choice;
    int loss_choice;
    cout << "================================================================\n";
    cout << "Provide your network interface by choosing one of the following: \n";
    cout << "1. enp\n";
    cout << "2. eth\n";
    cout << "3. wlan\n";
    cout << "4. lo\n";
    cout << "5. tun\n";
    cout << "================================================================\n";
    cout << "Your choice: \n";
    cin >> interface_choice;
    cout << "Now provide the packet loss value [%]: \n";
    cin >> loss_choice;
    
    switch(interface_choice){
      case 1:
            {
            string command1 = ("sudo tc qdisc add dev enp0s3 root netem loss " + to_string(loss_choice) + "%");
            system(command1.c_str());
            }
      break;
      case 2:
            {
            string command1 = ("sudo tc qdisc add dev enp0s3 root netem loss " + to_string(loss_choice) + "%");
            system(command1.c_str());
            }
      break;
      case 3:
            {
            string command1 = ("sudo tc qdisc add dev enp0s3 root netem loss " + to_string(loss_choice) + "%");
            system(command1.c_str());
            }
      break;
      case 4:
            {
            string command1 = ("sudo tc qdisc add dev enp0s3 root netem loss " + to_string(loss_choice) + "%");
            system(command1.c_str());
            }
      break;
      case 5:
            {
            string command1 = ("sudo tc qdisc add dev enp0s3 root netem loss " + to_string(loss_choice) + "%");
            system(command1.c_str());
            }
      break;
    }
    */

    // Create Client with selected transport backend
    // Client doesnt care about the actual transport implementation
	Client theClient(move(transport_backend));

    if (theClient.initialize_transmission(server_ip, port)) {
        cout << "[Main] Connection established to " << server_ip << ":" << port << ".\n";
        
        // The main application loop is executed within start_transmission().
        // Telemetry and messages are handled internally.
        // The function returns when the user decides to quit.
        theClient.start_transmission();
    }
    
    cout << "[Main] Client application terminated.\n";
    //string command_final = ("tc -p qdisc ls dev [to do]");
    return 0;
}
