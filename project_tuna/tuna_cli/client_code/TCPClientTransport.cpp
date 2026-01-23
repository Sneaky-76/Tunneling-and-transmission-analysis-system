#include "TCPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <cmath>		//fabs
//#include <chrono>               //for telemtery update

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
        telemetry_update();
	ssize_t snd = ::write(sockfd, data.data(), data.size());
        if(snd >= 0){
            stats.total_bytes_sent += snd;
            stats.total_packets_sent++;
        }
        return snd;
}

ssize_t TCPClientTransport::recieve(vector<uint8_t>& data){
	data.resize(512);	//our max byte count in buffer
	ssize_t recvdBytes = ::read(sockfd, data.data(), data.size());
	if(recvdBytes > 0){
	        stats.total_packets_received++;
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
	}
	stats.rtt_ms = rtt_val; //"new" is our old now

	stats.total_packets++;	//incrementing the packet count
}

void TCPClientTransport::telemetry_update() {
  auto now = std::chrono::steady_clock::now();
  
  struct tcp_info info;
  socklen_t info_len = sizeof(info);

  //getting kernel data
  if (getsockopt(this->sockfd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0) {

  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

  if (duration_ms >= 100) {
     double seconds = duration_ms / 1000.0;
  
  //in TCP we ask OS about packet loss
  if(this->stats.total_packets){  //are there packets? if so, then count the loss | _
        this->stats.packet_loss = (double)info.tcpi_total_retrans/(this->stats.total_packets +info.tcpi_total_retrans);
  }

    //throughput calulations  (all bytes sent in that particular measurent)
    long long bytes_diff = stats.total_bytes_sent - stats.last_total_bytes;
    this->stats.throughput_kbps = (bytes_diff * 8.0 / 1000.0) / seconds;    //dividing by 1000 because of kilo- (bytes)

    //goodput calculattion  (throughput - loss)
    this->stats.goodput_kbps = this->stats.throughput_kbps * (1.0 - this->stats.packet_loss);

    //everything back to "idle" from the new message perspective
    this->stats.last_check_time = now;
    }   //duration_ms
}   //getsockopt

}

    Telemetry TCPClientTransport::get_stats(){
        //this->stats.total_packets_sent = this->total_sent_requests;
        //this->stats.total_packets_received = this->total_received_responses;
	return stats;
    }


