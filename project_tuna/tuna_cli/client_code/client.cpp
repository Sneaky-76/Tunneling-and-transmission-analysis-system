#include <iostream>
#include <chrono>				//used for rtt measurement
#include <limits> 				// Required for clearing input buffer
#include <iomanip>            	//fixed & setprecision
#include "client.h"
#include "../core/telemetry.h"
#include <cstdlib>            	//system command

using namespace std;

//gemini was used in order to properly implement automatic "find user's interface" function
string Client::find_active_interface() {	//function determining with which interface should the program bind
  struct ifaddrs *addrs, *tmp;	//addrs holds teh start of the network interfaces list, tmp will pick one of them
  getifaddrs(&addrs);			//system call of "give me a list of every network interface available right now"
  tmp = addrs;					//starting the search for desired interface

  string interface_name = "lo"; //assuming loopback is default

  while (tmp) {					//if there is another interface, look for it
    //loooking for an interface that is up,has an IP, and is not the loopback (lo)
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {		//we're operating with IPv4
	//tmp->ifa_addr checks if the interface has an address assigned,
	//tmp->ifa_addr->sa_family == AF_INET checks if we're grabbing an IPv4 address
    string name = tmp->ifa_name;	//the the name of the interface
      if (name != "lo") {			//ignore loopback, we dont tunel through it
        interface_name = name;		//save the result
      break;						//found something? break out
      }
    }
  tmp = tmp->ifa_next;				//jumping to the next found interface in ifaddrs struct
  }

  freeifaddrs(addrs);				//memory leak prevention (because of heap allocation via getifaddr)
  return interface_name;			//return the result
}

Client::Client(unique_ptr<Transport> t) : transport(move(t)) {}

bool Client::initialize_transmission(const string& addr, uint16_t port){
    // Address storage for later usage (Traffic Generator needs this)
    this->server_ip = addr; 

    // Attempt to connect to the server
    if(!transport->connectTo(addr, port)){
        return false;
    }
    
    // NOTE: Background traffic setup is moved to start_transmission
    // to occur ONLY after successful authentication.

    return true;
}

bool Client::start_transmission(){

	// Gemini 3 Pro was used to help integrating authentication.
    // ---  AUTHENTICATION LOOP ---
    bool is_logged_in = false;

    while(!is_logged_in) {
        cout << "\n--- Authentication Required ---\n";
        cout << "1. Login\n";
        cout << "2. Register\n";
        cout << "3. Exit\n";
        cout << "Select option: ";

        int choice;
        // Input validation to prevent infinite loops on invalid char
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear n ewline from buffer

        if (choice == 3) {
            return false; // Exit application
        }

        string u, p, cmd;
        
        if (choice == 1) {
            cmd = "LOGIN";
            cout << "Username: "; cin >> u;
            cout << "Password: "; cin >> p;
        } else if (choice == 2) {
            cmd = "REGISTER";
            cout << "Username: "; cin >> u;
            cout << "Password: "; cin >> p;
        } else {
            cout << "Invalid option.\n";
            continue;
        }
        // Clearing buffer after string input before next I/O
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

        // Protocol message construction: "CMD user pass"
        string auth_msg = cmd + " " + u + " " + p;
        vector<uint8_t> payload(auth_msg.begin(), auth_msg.end());

        // Auth request transmission
        if(!transport->send(payload)) {
            cout << "[Error] Failed to send authentication request.\n";
            break;
        }

        // Waiting for server response
        vector<uint8_t> response_buff;
        if(transport->recieve(response_buff) > 0) {
            string resp(response_buff.begin(), response_buff.end());
            
            // Response analysis
            if (resp.find("AUTH_SUCCESS") != string::npos) {
                cout << "\n[Success] Authentication successful.\n";
                is_logged_in = true;
            } else if (resp.find("REG_SUCCESS") != string::npos) {
                cout << "\n[Success] Registration completed. Please login now.\n";
            } else {
                cout << "\n[Server] " << resp << "\n";
                cout << "Please try again.\n";
            }
        }
    }

    // Safety check:  if loop exited without login (e.g., break), return.
    if (!is_logged_in) return false;
  
    string interface = find_active_interface();	//get the user's interface automatically 
    
    string p_drop_ans;
    string p_drop_cmnd_start;
    string p_drop_cmnd_stop;
    bool was_here_already = false;
    cout << "Provide packet drop value [%]: ";
    cin >> p_drop_ans;
    int p_drop_val = stoi(p_drop_ans);
    if(p_drop_val >= 100){		//100% is the maximum limit
      p_drop_val = 100;
      was_here_already = true;
    }else if(p_drop_val <= 0){	//0% is the minimum limit
      p_drop_val = 0;
      was_here_already = true;
    }
    
    if(was_here_already){   //if there is an attempt to remove something that does not exist, there will be an error
      p_drop_cmnd_stop = ("sudo tc qdisc del dev " + interface + " root");  //deleting the changes made via command
      system(p_drop_cmnd_stop.c_str());
    }
    p_drop_cmnd_start = ("sudo tc qdisc add dev " + interface + " root netem loss " + to_string(p_drop_val) + "%");
    system(p_drop_cmnd_start.c_str());

    
	// Gemini 3 Pro was used to help implement background traffic  generation.
    // --- BACKGROUND TRAFFIC SECTION ---
    
    char run_bg;
    cout << "\nStart background traffic generator? (y/n): ";
    cin >> run_bg;
    cin.ignore(); // Input buffer clearing

    if(run_bg == 'y' || run_bg == 'Y') {
        cout << "[Client] Starting UDP flood to " << this->server_ip << ":9999" << endl;
        
        //  Setup using the stored server IP
        if(bg_traffic.setup(this->server_ip, 9999)) {
            // Start generation: 1400 bytes packet size, 0ms interval (max speed)
            bg_traffic.start(1400, 0); 
        }
    }
  
    while(true){ 
        string input;
        cout << "\nEnter the message (or type q to quit): \n";
        getline(cin, input);
        
        if(input == "q" || input == "Q") break;  

        vector<uint8_t> payload(input.begin(), input.end());

        auto start_rtt_time_measurement = std::chrono::steady_clock::now();
        
        // Message is transmitted
        if(transport->send(payload) <= 0){
             cout << "Message transmission failed.\n";
             break;
        }

        cout << "Message sent, waiting for confirmation . . .\n";

        vector<uint8_t> response_buff;
        ssize_t bytes = transport->recieve(response_buff);
        
        auto end_rtt_time_measurement = std::chrono::steady_clock::now();
        
        // Duration is calculated
        auto time_elapsed = end_rtt_time_measurement - start_rtt_time_measurement;
        std::chrono::duration<double, std::milli> time_elapsed_in_ms = time_elapsed;    
        double rtt_val = time_elapsed_in_ms.count();
        
        // Internal RTT state is updated
        transport->update_rtt_value(rtt_val);
        
        if(bytes > 0){
            string confirmation(response_buff.begin(), response_buff.end());
            cout << "\n[Server response]: " << confirmation << endl;
            
            // FULL TELEMETRY DISPLAY (Migrated from main.cpp)
            // Stats are retrieved from the transport layer
            Telemetry stats = transport->get_stats();

            cout << "Measured RTT: " << rtt_val << " ms" << endl;
            // Note: Jitter is accessed from the stats structure
            cout << "Measured jitter: " << stats.jitter << " ms" << endl; 
            cout << "MTU: " << stats.mtu << " bytes" << endl;
            
            // Packet loss, Throughput, and Goodput are displayed with precision
            cout << "Packet loss: " << fixed << setprecision(3) << (stats.packet_loss) * 100.0 << " %\n";
            cout << "Throughput: " << stats.throughput_kbps << " kbps\n";
            cout << "Goodput: " << stats.goodput_kbps << " kbps\n";

        } else {
            cout << "\n[Error]: No response recieved.\n";
        }

        // Telemetry state is updated/reset for the next measurement window
        // This corresponds to 'active_transport->telemetry_update()' from main.cpp
        transport->telemetry_update();
    }

    bg_traffic.stop();
    p_drop_cmnd_stop = ("sudo tc qdisc del dev " + interface + " root");  //deleting the changes (if leaving the program completely)
    system(p_drop_cmnd_stop.c_str());
    return true;
}

void Client::close_transmission(){
    transport->close_connection();
}
