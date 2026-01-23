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
        sockaddr_in destination_addr;
        sockaddr_in source_addr;
    public:
        UDPClientTransport();
        explicit UDPClientTransport(int existing_fd);
        ~UDPClientTransport();


        bool connectTo(const string &addr, uint16_t port) override;
        ssize_t send(const vector<uint8_t> &data) override;
        ssize_t recieve(vector<uint8_t> &data) override;
        void close_connection() override;
};


#endif