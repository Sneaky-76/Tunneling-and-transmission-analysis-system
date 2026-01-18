#include "SCTPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/sctp.h> 	// Biblioteka SCTP
#include <cmath>		//fabs
#include <cstring>              //memset

SCTPClientTransport::SCTPClientTransport() : sockfd(-1) {}
SCTPClientTransport::SCTPClientTransport(int existing_fd) : sockfd(existing_fd) {}
SCTPClientTransport::~SCTPClientTransport() { close_connection(); }

void SCTPClientTransport::update_mtu(){ 

        struct sctp_status status{};
        socklen_t len = sizeof(status);

        if(getsockopt(sockfd, IPPROTO_SCTP, SCTP_STATUS, &status, &len) == 0) 
                stats.mtu = status.sstat_primary.spinfo_mtu;
}

bool SCTPClientTransport::connectTo(const string& addr, uint16_t port) {
    // Tworzymy gniazdo SCTP (One-to-One style)
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) { perror("SCTP Socket error"); return false; }

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) return false;

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("SCTP Connect error");
        return false;
    }

    return true;
}



ssize_t SCTPClientTransport::send(const vector<uint8_t>& data) {
	
	update_mtu();
    ssize_t snd = ::sctp_sendmsg(this->sockfd, data.data(), data.size(), NULL,0,0,0,0,0,0);
    //works, just testing sctp_sendmsg, previously: ::send(sockfd, data.data(), data.size(), 0);
    
    if(snd >= 0){
      this->stats.total_packets_sent += snd;
    }
  return snd;
}

ssize_t SCTPClientTransport::recieve(vector<uint8_t>& data) {
    data.resize(512);
    ssize_t recvd = ::sctp_recvmsg(this->sockfd, data.data(), data.size(),NULL,0,0,0);//::recv(sockfd, data.data(), data.size(), 0);
    if (recvd >= 0){
      this->stats.total_packets_recieved++;
      data.resize(recvd);
    }
    return recvd;
}

void SCTPClientTransport::close_connection() {
    if (sockfd >= 0) { ::close(sockfd); sockfd = -1; }
}

void SCTPClientTransport::update_rtt_value(double rtt_val){
	if(stats.total_packets > 0){
		stats.jitter = fabs(rtt_val - stats.rtt_ms); 	//new - old rtt, jitter measurement
								//because its strictly tied to rtt
	}
	stats.rtt_ms = rtt_val; //"new" is our old now

	stats.total_packets++;	//incrementing the packet count
}

void SCTPClientTransport::telemetry_update() {
    auto now = std::chrono::steady_clock::now();

    struct sctp_assoc_stats assoc_stat;
    socklen_t assoc_len = sizeof(assoc_stat);

    //getting kernel data
    if(getsockopt(this->sockfd, IPPROTO_SCTP, SCTP_GET_ASSOC_STATS, &assoc_stat, &assoc_len) == 0){
      this->stats.packet_loss = assoc_stat.sas_rtxchunks;
    }
  
    //he time since the last check calculation (used for counting throughput and goodput with seconds)
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

    //update after some time has passed to avoid noise
    if (duration_ms > 100) {
    //calculate bytes sent based on packets (standard byte MTU os 1500 from previous implementations)
      long long current_total_bytes = stats.total_packets_sent*1500;
      long long bytes_diff = current_total_bytes - stats.last_total_bytes;
    
    //calculation: (bytes*8/1000)/seconds
      double seconds = duration_ms/1000.0;
      stats.throughput_kbps = (bytes_diff*8.0/1000.0) / seconds;

    //goodput = throughput - the lost/retransmitted parts of the message 
      if (stats.throughput_kbps > 0) {
        double loss_ratio = (assoc_stat.sas_rtxchunks*1500*8.0/1000.0)/seconds;//stats.get_loss_percent() / 100.0;
        stats.goodput_kbps = stats.throughput_kbps - loss_ratio;//* (1.0 - loss_ratio);
      } else {
        stats.throughput_kbps=0;
      }

      //updating time markers for the next loop
      stats.last_total_bytes = current_total_bytes;
      stats.last_check_time = now;
    }
}

Telemetry SCTPClientTransport::get_stats(){
	return stats;
}

