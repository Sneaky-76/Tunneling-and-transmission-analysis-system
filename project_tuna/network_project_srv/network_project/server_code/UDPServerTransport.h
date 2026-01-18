#ifndef UDPSERVER_TRANSPORT_H
#define UDPSERVER_TRANSPORT_H
#include <string>
#include <vector>
#include <netinet/in.h>
#include "../core/ServerTransport.h"
#include <memory>
///

using std::string;
using std::vector;

class UDPServerTransport : public ServerTransport {
private:
    int sockfd;
public:
    UDPServerTransport();
    ~UDPServerTransport();

    bool bindAndListen(uint16_t port) override;
    std::unique_ptr<Transport> acceptClient() override;
    ssize_t receiveFrom(vector<uint8_t> &data, sockaddr_in &from);
    ssize_t sendTo(const std::vector<uint8_t> &data, const sockaddr_in &to);
    void close_connection() override;
};

#endif