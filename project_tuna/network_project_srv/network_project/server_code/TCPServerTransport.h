#ifndef TCPSERVER_TRANSPORT_H
#define TCPSERVER_TRANSPORT_H

#include "../core/ServerTransport.h"
#include <memory>

using namespace std;

class TCPServerTransport : public ServerTransport {
private:
    int listenfd;
public:
    TCPServerTransport();
    ~TCPServerTransport();

    // Tylko te metody są wymagane przez ServerTransport:
    bool bindAndListen(uint16_t port) override;
    unique_ptr<Transport> acceptClient() override;
    void close_connection() override;
};

#endif