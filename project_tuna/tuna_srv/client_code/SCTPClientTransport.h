#ifndef SCTP_CLIENT_TRANSPORT_H
#define SCTP_CLIENT_TRANSPORT_H

#include "../core/transport.h"
#include <netinet/in.h>

class SCTPClientTransport : public Transport {
private:
    int sockfd;
public:
    SCTPClientTransport();
    explicit SCTPClientTransport(int existing_fd); // Ważne dla serwera!
    ~SCTPClientTransport();

    bool connectTo(const string& addr, uint16_t port) override;
    ssize_t send(const vector<uint8_t>& data) override;
    ssize_t recieve(vector<uint8_t>& data) override;
    void close_connection() override;
};
#endif