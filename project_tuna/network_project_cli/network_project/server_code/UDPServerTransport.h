#ifndef UDPSERVER_TRANSPORT_H
#define UDPSERVER_TRANSPORT_H
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "../core/ServerTransport.h"
#include <memory>

using namespace std;

class UDPServerTransport : public ServerTransport {
private:
    
public:
    UDPServerTransport();
    ~UDPServerTransport();

    bool bind(uint16_t port);
    ssize_t receiveFrom(vector<uint8_t> &data, sockaddr_in &from);
    ssize_t sendTo(const vector<uint8_t> &data, const sockaddr_in &to);
    void close_connection() override;
};

#endif