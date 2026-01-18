#include "TCPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <cmath>		//fabs

using namespace std;

TCPClientTransport::TCPClientTransport() : sockfd(-1) { stats.rtt_ms = 0.0, stats.jitter = 0.0;}
TCPClientTransport::TCPClientTransport(int existing_fd) : sockfd(existing_fd) {}
TCPClientTransport::~TCPClientTransport() { close_connection(); }

void TCPClientTransport::update_mtu(){
        int mtu_val=0;
        socklen_t mtu_length = sizeof(mtu_val);
        if(getsockopt(sockfd, IPPROTO_IP, IP_MTU, &mtu_val, &mtu_length) == 0){
                stats.mtu = mtu_val;
        }else{
                perror("Obtaining MTU have failed.");
        }
}  

bool TCPClientTransport::connectTo(const string& addr, uint16_t port){
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Socket error: ");
		return 0;
	}

	struct sockaddr_in servaddr{};
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0){	//c_str provides a pointer
									//to the first char
		//inet_pton: 0 if src not found, -1 if error. warning about both
		perror("Pton error: ");
		return 0;
	}

	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){ //0 = success, -1 = error
		perror("Connect error: ");
		return 0;
	}
	return 1;
}

ssize_t TCPClientTransport::send(const vector<uint8_t>& data){
	
	update_mtu();
	ssize_t snd = ::write(sockfd, data.data(), data.size());//::send(sockfd, data.data(), data.size(), 0);
    if(snd >= 0){
      this->stats.total_packets_sent += snd;
    }
  return snd;

}

ssize_t TCPClientTransport::recieve(vector<uint8_t>& data){
	data.resize(512);	//our max byte count in buffer
	ssize_t recvdBytes = ::read(sockfd, data.data(), data.size());//::recv(sockfd, data.data(), data.size(), 0);
	if(recvdBytes > 0){
	        this->stats.total_packets_recieved++;
		data.resize(recvdBytes);
	}
	return  recvdBytes;
}

void TCPClientTransport::close_connection(){
	if(sockfd >= 0){	//checking if the socket is stil open
		::close(sockfd);
		sockfd = -1; 	//prevention from closing multiple times
	}
}

void TCPClientTransport::update_rtt_value(double rtt_val){
	if(stats.total_packets > 0){
		stats.jitter = fabs(rtt_val - stats.rtt_ms); //new - old rtt
		//cout << "Old val: " << stats.rtt_ms << ", New one:" << rtt_val << endl;
	}
	stats.rtt_ms = rtt_val; //"new" is our old now

	stats.total_packets++;	//incrementing the packet count
}

void TCPClientTransport::telemetry_update() {
    auto now = std::chrono::steady_clock::now();

    struct tcp_info info;
    socklen_t info_len = sizeof(info); // Critical initialization

    //getting kernel data
    if (getsockopt(this->sockfd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0) {
      this->stats.packet_loss = info.tcpi_retransmits;
    }
  
    //he time since the last check calculation (used for counting throughput and goodput with seconds)
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

    //update after some time has passed to avoid noise
    if (duration_ms > 100) {
    //calculate bytes sent based on packets (standard byte MTU os 1500 from previous implementations)
      long long current_total_bytes = stats.total_packets_sent*1500;
      long long bytes_diff = current_total_bytes - stats.last_total_bytes;

    //calculation: (bytes*8/1000)/seconds
      double seconds = duration_ms/1000.0;  //i'd normally say *1000, but this one works i guess
      stats.throughput_kbps = (bytes_diff*8.0/1000.0)/seconds;  //divide by 1000 because kilo-

    //goodput = throughput - the lost/retransmitted parts of the message
      if (stats.throughput_kbps > 0) {
        double loss_ratio = stats.get_loss_percent()/100.0;
        stats.goodput_kbps = stats.throughput_kbps*(1.0 - loss_ratio);
      }

      //updating time markers for the next loop
      stats.last_total_bytes = current_total_bytes;
      stats.last_check_time = now;
    }
}

Telemetry TCPClientTransport::get_stats(){
	return stats;	//returning statistics for Telemetry
}



