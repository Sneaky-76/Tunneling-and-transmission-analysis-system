#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "../core/transport.h"
#include <netinet/in.h>		//includes the definition of sockaddr_in,
				//IPPROTO & many others
#include <vector>


class TCPClientTransport : public Transport {
private:
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
	std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data) override;
	std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data) override;

};

#endif
