#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "../core/transport.h"
#include <netinet/tcp.h>        //contains tcp_info structure
#include <netinet/in.h>		//includes the definition of sockaddr_in,
				//IPPROTO & many others

class TCPClientTransport : public Transport {
private:
	int sockfd;
	Telemetry stats;
	
	//for packet loss measurement
        uint32_t total_sent_requests=0;
        uint32_t total_received_responses=0;
        
        uint32_t last_sent_count = 0;
        uint32_t last_recvd_count = 0;
public:
	TCPClientTransport();
	explicit TCPClientTransport(int existing_fd);
	~TCPClientTransport();

	bool connectTo(const string& addr, uint16_t port) override;
    ssize_t send(const vector<uint8_t>& data) override;
    ssize_t recieve(vector<uint8_t>& data) override;
    void close_connection() override;
	
	void update_rtt_value(double rtt_val) override;
	void update_mtu() override;
	Telemetry get_stats() override;
	
        //goodput, throughput & packet loss/retransmissions
	void telemetry_update() override;
	
};

#endif
