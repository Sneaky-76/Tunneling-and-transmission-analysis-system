#include "server.h"
#include "../core/ServerTransport.h"
#include <iostream>
#include <thread>
#include <syslog.h>
#include <sstream> // Added for command parsing
#include "UDPServerTransport.h"
#include <arpa/inet.h>

// Gemini 3 Pro was used to help implement authentication and DatabaseManager integration.

/*
 * Constructor: Initializes transport, logger, and the database manager.
 * Note: db_manager is initialized with the filename here in the initialization list.
 */
Server::Server(unique_ptr<ServerTransport> st) 
    : transport(move(st)), db_manager("users_db.txt"), is_running(false) {
    openlog("NetworkAnalyzerServ", LOG_PID, LOG_USER);
}

Server::~Server(){
    close_transmission();
    closelog();
}

bool Server::initialize_transmission(const string& addr, uint16_t port){
    if(transport->bindAndListen(port)){
        syslog(LOG_INFO, "Server bound to port %u resulted in a success", port);
        is_running = true;
        return true;
    }
    syslog(LOG_ERR, "Server failed to bound to port %u", port);
    return false;
}

/*
 * Handles a single client session with an Authentication State Machine.
 * Flow:
 * 1. Unauthenticated state (Only LOGIN/REGISTER commands allowed).
 * 2. Authenticated state (Full access to tools).
 */
void Server::handle_client_session(unique_ptr<Transport> cli_conn){
    syslog(LOG_INFO, "New client connected. Session started.");
    
    vector<uint8_t> buff;
    bool is_logged_in = false; 
    string current_user = "";

    // Main connection loop
    while(true) {
        buff.clear();
        ssize_t bytes = cli_conn->recieve(buff);

        // Check for disconnection or error
        if (bytes <= 0) {
            syslog(LOG_INFO, "Client disconnected.");
            break; 
        }

        string msg(buff.begin(), buff.end());
        
        // Cleanup: Remove trailing newline characters/garbage
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }

        // --- AUTHENTICATION ---
        // If the user is not logged in he will get intercepted here.
        if (!is_logged_in) {
            stringstream ss(msg);
            string command, u, p;
            ss >> command >> u >> p; 

            string response;

            if (command == "LOGIN") {
                if (db_manager.authenticate(u, p)) {
                    is_logged_in = true;
                    current_user = u;
                    response = "AUTH_SUCCESS";
                    syslog(LOG_INFO, "User logged in: %s", u.c_str());
                } else {
                    response = "AUTH_FAIL";
                    syslog(LOG_WARNING, "Failed login attempt for: %s", u.c_str());
                }
            } 
            else if (command == "REGISTER") {
                if (!u.empty() && !p.empty()) {
                    if (db_manager.registerUser(u, p)) {
                        response = "REG_SUCCESS";
                        syslog(LOG_INFO, "New user registered: %s", u.c_str());
                    } else {
                        response = "REG_FAIL: User exists";
                    }
                } else {
                    response = "REG_FAIL: Invalid data";
                }
            } 
            else {
                response = "ERROR: Authentication required. Usage: LOGIN <user> <pass> or REGISTER <user> <pass>";
            }

            // Send auth response and skip the rest of the loop logic (continue)
            vector<uint8_t> r(response.begin(), response.end());
            cli_conn->send(r);
            continue; 
        }

        // --- AUTHORIZED SECTION ---
        // Code below is executed ONLY for loggedin users
        
        cout << "[Server][" << current_user << "] Recieved: " << msg << endl;

        string response = "ACK: message has been recieved by the server.";
        vector<uint8_t> responseData(response.begin(), response.end());

        cli_conn->send(responseData);
        syslog(LOG_DEBUG, "Recieved %zd bytes.", bytes);
    }
    
    cli_conn->close_connection();
}

void Server::start_transmission(){
    // Initialization of the background traffic sink on port 9999
    bg_sink.start(9999);

    cout << "Server running and awaiting for connection . . .\n";

    // Special handling for UDP transport (stateless)
    if (auto* udp = dynamic_cast<UDPServerTransport*>(transport.get())) {

        vector<uint8_t> buffer;
        sockaddr_in from{};

        while (is_running) {
            ssize_t n = udp->receiveFrom(buffer, from);
            if (n <= 0) continue;

            // FIX: Logging is commented out to prevent console lag during flood
            /*
            cout << "[UDP] Packet from "
                 << inet_ntoa(from.sin_addr)
                 << ":" << ntohs(from.sin_port)
                 << " size=" << n << endl;
            */

            // FIX: UDP Logic updated to support Authentication instead of pure Echo
            string msg(buffer.begin(), buffer.end());
            
            // Cleanup: Remove trailing newline characters/garbage
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
                msg.pop_back();
            }

            stringstream ss(msg);
            string command, u, p;
            ss >> command; // Read first word

            string response;

            // Authentication logic adapted for UDP
            if (command == "LOGIN") {
                ss >> u >> p;
                if (db_manager.authenticate(u, p)) {
                     response = "AUTH_SUCCESS";
                     syslog(LOG_INFO, "[UDP] User logged in: %s", u.c_str());
                } else {
                     response = "AUTH_FAIL";
                }
            }
            else if (command == "REGISTER") {
                ss >> u >> p;
                if (db_manager.registerUser(u, p)) {
                    response = "REG_SUCCESS";
                } else {
                    response = "REG_FAIL";
                }
            }
            else {
                // If not a command, it is treated as a chat message
                cout << "[UDP Chat]: " << msg << endl;
                response = "ACK: message has been recieved by the server.";
            }

            vector<uint8_t> respData(response.begin(), response.end());
            udp->sendTo(respData, from); 
        }
        return;
    }
    
    // Standard connection-oriented handling (TCP/SCTP)
    while(is_running){
        unique_ptr<Transport> client = transport->acceptClient();

        if(client){
            // Thread created for simultaneous client handling
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