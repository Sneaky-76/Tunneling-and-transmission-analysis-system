#ifndef SERVER_TRANSPORT_H
#define SERVER_TRANSPORT_H

#include "transport.h"
#include <memory>
#include <cstdint>

using namespace std;

class ServerTransport {
public:
    virtual ~ServerTransport() = default;
    virtual bool bindAndListen(uint16_t port) = 0;
    virtual unique_ptr<Transport> acceptClient() = 0;
    virtual void close_connection() = 0;
    
};

#endif