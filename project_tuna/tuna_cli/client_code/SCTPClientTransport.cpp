#include "SCTPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/sctp.h> 	// Biblioteka SCTP
#include <cmath>		//fabs

SCTPClientTransport::SCTPClientTransport() : sockfd(-1) { stats.rtt_ms = 0.0, stats.jitter = 0.0;}
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
	
    this->telemetry_update();
    this->update_mtu();
    ssize_t snd = ::sctp_sendmsg(this->sockfd, data.data(), data.size(), NULL,0,0,0,0,0,0);
    //works, just testing sctp_sendmsg, previously: ::send(sockfd, data.data(), data.size(), 0);
    
    if(snd >= 0){
      this->stats.total_bytes_sent += snd;
      this->stats.total_packets_sent++;
    }
  return snd;
}

ssize_t SCTPClientTransport::recieve(vector<uint8_t>& data) {
    data.resize(512);
    ssize_t recvd = ::sctp_recvmsg(this->sockfd, data.data(), data.size(),NULL,0,0,0);//::recv(sockfd, data.data(), data.size(), 0);
    if (recvd >= 0){
      this->stats.total_packets_received++;
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
  if(getsockopt(this->sockfd, IPPROTO_SCTP, SCTP_GET_ASSOC_STATS, &assoc_stat, &assoc_len) == 0) {

  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

  if (duration_ms >= 100) {
     double seconds = duration_ms / 1000.0;
  
  //in SCTP we ask OS about packet loss
  if(this->stats.total_packets>0){
    this->stats.packet_loss = (double)assoc_stat.sas_rtxchunks/(this->stats.total_packets+assoc_stat.sas_rtxchunks);
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

Telemetry SCTPClientTransport::get_stats(){
	return stats;
}

