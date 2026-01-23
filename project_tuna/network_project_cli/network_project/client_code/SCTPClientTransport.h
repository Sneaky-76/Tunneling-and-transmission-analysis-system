#ifndef SCTP_CLIENT_TRANSPORT_H
#define SCTP_CLIENT_TRANSPORT_H

#include "../core/transport.h"
#include "../core/telemetry.h"
#include <netinet/in.h>
#include <vector>
class SCTPClientTransport : public Transport {
private:
    int sockfd;
    Telemetry stats;
public:
    SCTPClientTransport();
    explicit SCTPClientTransport(int existing_fd); // Ważne dla serwera!
    ~SCTPClientTransport();

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
