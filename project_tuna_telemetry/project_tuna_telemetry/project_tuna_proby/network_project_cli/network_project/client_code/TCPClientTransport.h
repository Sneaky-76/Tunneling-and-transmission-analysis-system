#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "../core/transport.h"
#include "../core/telemetry.h"
#include <netinet/tcp.h>        //contains tcp_info structure
#include <netinet/in.h>		//includes the definition of sockaddr_in,
				//IPPROTO & many others

class TCPClientTransport : public Transport {
private:
	//bool isClient;		//for packet loss measurement (to differ between client and server)
	int sockfd;
	Telemetry stats;
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

	void telemetry_update() override;  //int sockfd

};

#endif
