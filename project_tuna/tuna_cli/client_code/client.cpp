#include <iostream>
#include <chrono>			//add1
#include "client.h"
#include "../core/telemetry.h"

using namespace std;

Client::Client(unique_ptr<Transport> t) : transport(move(t)) {}

bool Client::initialize_transmission(const string& addr, uint16_t port){
    // Address storage for later usage
    this->server_ip = addr; 

    // Attempt to connect to the server
    if(!transport->connectTo(addr, port)){
        return false;
    }

    // --- BACKGROUND TRAFFIC SECTION ---
    
    char run_bg;
    cout << "Start background traffic generator? (y/n): ";
    cin >> run_bg;
    cin.ignore(); // Input buffer clearing

    if(run_bg == 'y' || run_bg == 'Y') {
        cout << "[Client] Starting UDP flood to " << this->server_ip << ":9999" << endl;
        
        // Setup using the stored server IP
        if(bg_traffic.setup(this->server_ip, 9999)) {
            // Start generation: 1400 bytes packet size, 0ms interval (max speed)
            bg_traffic.start(1400, 0); 
        }
    }
    // --------------------------------------------------

    return true;
}
bool Client::start_transmission(){

//while(1){

	string input;
	cout << "Enter the message (or type q to quit): \n";
	getline(cin, input);
	if(input == "q" || input == "Q")
		return false;	

	vector<uint8_t> payload(input.begin(), input.end());

	auto start_rtt_time_measurement = std::chrono::steady_clock::now();
	transport->send(payload);

	cout << "Message sent, waiting for confirmation . . .\n";

	vector<uint8_t> response_buff;
	ssize_t bytes = transport->recieve(response_buff);
	auto end_rtt_time_measurement = std::chrono::steady_clock::now();

	auto time_elapsed = end_rtt_time_measurement - start_rtt_time_measurement;
	std::chrono::duration<double, std::milli> time_elapsed_in_ms = time_elapsed; 	//time in ms (double initialised in ms)
	double rtt_val = time_elapsed_in_ms.count();
	transport->update_rtt_value(rtt_val);
	
	if(bytes > 0){
		string confirmation(response_buff.begin(), response_buff.end());
		cout << "\n[Server response]: " << confirmation << endl;
	} else {
		cout << "\n[Error]: No response recieved.\n";
		//return false;
	}

	cout << "\nRecieved message consists of: " << response_buff.size() << " bytes.\n";
	 return true;
}

//}	//while(1)

void Client::close_transmission(){
	transport->close_connection();
}
