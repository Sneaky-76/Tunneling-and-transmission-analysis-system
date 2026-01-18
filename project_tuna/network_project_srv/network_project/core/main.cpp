#include <iostream>
#include <memory>
#include <string>

// Add includes for transport implementations
#include "../server_code/server.h"
#include "../server_code/TCPServerTransport.h"
#include "../server_code/SCTPServerTransport.h"
#include "../server_code/UDPServerTransport.h"

using namespace std;

int main(int argc, char* argv[]) {

    // Default to TCP if no argument is given
    string proto = (argc >= 2) ? argv[1] : "tcp";
    uint16_t port = 575;

    // Pointers to ServerTransport backend
    unique_ptr<ServerTransport> server_backend;

    // Choice of transport based on protocol
    if (proto == "tcp") {
        cout << "[Server Main] Turning on TCP server...\n";
        server_backend = make_unique<TCPServerTransport>();
    } 
    else if (proto == "sctp") {
        cout << "[Server Main] Turning on SCTP server...\n";
        server_backend = make_unique<SCTPServerTransport>();
    }
    else if (proto == "udp") {
        cout << "[Server Main] Turning on UDP server...\n";
        server_backend = make_unique<UDPServerTransport>();
    }
    else {
        cerr << "[Error] Unknown protocol: " << proto << endl;
        return 1;
    }

    // Forward the selected transport to the Server
    Server theServer(move(server_backend));

    if (theServer.initialize_transmission("", port)) {
        cout << "[Server Main] Listening on port " << port << ".\n";
        theServer.start_transmission();
    } else {
        cerr << "[Server Main] Failed to start server.\n";
        return 1;
    }

    return 0;
}