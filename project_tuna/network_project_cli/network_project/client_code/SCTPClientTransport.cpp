#include "SCTPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/sctp.h> 	// Biblioteka SCTP
#include <cmath>		//fabs

SCTPClientTransport::SCTPClientTransport() : sockfd(-1) {}
SCTPClientTransport::SCTPClientTransport(int existing_fd) : sockfd(existing_fd) {}
SCTPClientTransport::~SCTPClientTransport() { close_connection(); }

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
    return ::send(sockfd, data.data(), data.size(), 0);
}

ssize_t SCTPClientTransport::recieve(vector<uint8_t>& data) {
    data.resize(512);
    ssize_t recvd = ::recv(sockfd, data.data(), data.size(), 0);
    if (recvd >= 0) data.resize(recvd);
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

Telemetry SCTPClientTransport::get_stats(){
	return stats;
}
