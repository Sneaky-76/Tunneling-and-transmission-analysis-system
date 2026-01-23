#ifndef SCTPSERVER_TRANSPORT_H
#define SCTPSERVER_TRANSPORT_H
#include "../core/ServerTransport.h"

class SCTPServerTransport : public ServerTransport {
private:
    int listenfd;
public:
    SCTPServerTransport();
    ~SCTPServerTransport();
    bool bindAndListen(uint16_t port) override;
    unique_ptr<Transport> acceptClient() override;
    void close_connection() override;
};
#endif