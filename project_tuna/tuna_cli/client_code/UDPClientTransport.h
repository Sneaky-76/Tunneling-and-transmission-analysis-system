#ifndef UDPCLIENTTRANSPORT_H
#define UDPCLIENTTRANSPORT_H
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "../core/transport.h" 
#include <netinet/in.h>


class UDPClientTransport : public Transport {
    private:
        int sockfd;
        Telemetry stats;
        sockaddr_in destination_addr;
        sockaddr_in source_addr;
        
        //for packet loss measurement
        uint32_t total_sent_requests=0;
        uint32_t total_received_responses=0;
        
        uint32_t last_sent_count = 0;
        uint32_t last_recvd_count = 0;

    public:
        UDPClientTransport();
        explicit UDPClientTransport(int existing_fd);
        ~UDPClientTransport();


        bool connectTo(const string &addr, uint16_t port) override;
        ssize_t send(const vector<uint8_t> &data) override;
        ssize_t recieve(vector<uint8_t> &data) override;
        void close_connection() override;
        
        //1st telemetry update
        void update_rtt_value(double rtt_val) override;
        void update_mtu() override;
	Telemetry get_stats() override;
        
        //goodput, throughput & packet loss/retransmissions
	void telemetry_update() override;
};

#endif
