#include <iostream>
#include "client.h"

using namespace std;

Client::Client(unique_ptr<Transport> t) : transport(move(t)) {}

bool Client::initialize_transmission(const string& addr, uint16_t port){
	return transport->connectTo(addr, port);
}

void Client::start_transmission(){
	string input;
	cout << "Enter the message: \n";
	getline(cin, input);
	
	vector<uint8_t> payload(input.begin(), input.end());

	//vector<uint8_t> payload = {'S','o','m','e','t','h','i','n','g'};

	transport->send(payload);
	cout << "Message sent, waiting for confirmation . . .\n";

	vector<uint8_t> response_buff;
	ssize_t bytes = transport->recieve(response_buff);

	if(bytes > 0){
		string confirmation(response_buff.begin(), response_buff.end());
		cout << "\n[Server response]: " << confirmation << endl;
	} else {
		cout << "\n[Error]: No response recieved.\n";
	}

	cout << "\nRecieved message consists of: " << response_buff.size() << " bytes.\n";
	 
}

void Client::close_transmission(){
	transport->close_connection();
}
