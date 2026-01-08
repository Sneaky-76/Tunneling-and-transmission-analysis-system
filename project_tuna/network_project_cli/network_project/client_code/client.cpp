#include <iostream>
#include <chrono>			//add1
#include "client.h"
#include "../core/telemetry.h"

using namespace std;

Client::Client(unique_ptr<Transport> t) : transport(move(t)) {}

bool Client::initialize_transmission(const string& addr, uint16_t port){
	return transport->connectTo(addr, port);
}

bool Client::start_transmission(){

//while(1){
	string input;
	cout << "Enter the message (or type q to quit): \n";
	getline(cin, input);
	if(input == "q" || input == "Q")
		return false;	

	vector<uint8_t> payload(input.begin(), input.end());

	//vector<uint8_t> payload = {'S','o','m','e','t','h','i','n','g'};

	auto start_rtt_time_measurement = std::chrono::steady_clock::now(); //add2
	transport->send(payload);

	cout << "Message sent, waiting for confirmation . . .\n";

	vector<uint8_t> response_buff;
	ssize_t bytes = transport->recieve(response_buff);
	auto end_rtt_time_measurement = std::chrono::steady_clock::now(); //add3

	auto time_elapsed = end_rtt_time_measurement - start_rtt_time_measurement;
	//double rtt_val = static_cast<double>(time_elapsed.count());
	std::chrono::duration<double, std::milli> time_elapsed_in_ms = time_elapsed; 	//time in ms (double initialised in ms)
	double rtt_val = time_elapsed_in_ms.count();
	transport->update_rtt_value(rtt_val);
	
	if(bytes > 0){
		string confirmation(response_buff.begin(), response_buff.end());
		cout << "\n[Server response]: " << confirmation << endl;
	} else {
		cout << "\n[Error]: No response recieved.\n";
		return false;
	}

	cout << "\nRecieved message consists of: " << response_buff.size() << " bytes.\n";
	 return true;
}

//}	//while(1)

void Client::close_transmission(){
	transport->close_connection();
}
