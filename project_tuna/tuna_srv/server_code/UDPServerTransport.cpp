#include "UDPServerTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <netinet/in.h>
#include <vector>
using std::vector;
using std::string;

UDPServerTransport::UDPServerTransport(): sockfd(-1){}
UDPServerTransport::~UDPServerTransport(){close_connection();}

std::unique_ptr<Transport> UDPServerTransport::acceptClient(){
return nullptr;
}


bool UDPServerTransport::bindAndListen(uint16_t port){
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return false;
    }
    struct sockaddr_in servaddr {};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(::bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("Bind error");
        return false;
    }
    return true;
}



ssize_t UDPServerTransport::receiveFrom(vector<uint8_t> &data,sockaddr_in &from ){
    data.resize(65507);
    socklen_t length = sizeof(from);
    ssize_t n = ::recvfrom(sockfd, data.data(),data.size(),0,(struct sockaddr*)&from ,&length); // rzutowanie wskażnika czyli traktowanie sockaddr_in jako sockaddr
    if ( n>=0) data.resize(n);
    return n;

}

ssize_t UDPServerTransport::sendTo(const std::vector<uint8_t> &data, const sockaddr_in &to) {
    return ::sendto(sockfd, data.data(), data.size(), 0, (struct sockaddr*)&to, sizeof(to));
}

void UDPServerTransport::close_connection() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}
