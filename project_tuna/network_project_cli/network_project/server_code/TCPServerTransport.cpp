#include "TCPServerTransport.h"
#include "../client_code/TCPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/in.h>

#define LISTENQ 5

TCPServerTransport::TCPServerTransport() : listenfd(-1){}
TCPServerTransport::~TCPServerTransport() { close_connection(); }

bool TCPServerTransport::bindAndListen(uint16_t port){
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        perror("Socket error");
        return false;
    }
    
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("Bind error");
        return false;
    }
    if(listen(listenfd, LISTENQ) < 0){
        perror("Listen error");
        return false;
    }
    return true;
}

unique_ptr<Transport> TCPServerTransport::acceptClient(){
    struct sockaddr_in cliaddr{};
    socklen_t cli_len = sizeof(cliaddr);

    int clifd = accept(listenfd, (struct sockaddr*)&cliaddr, &cli_len);
    if(clifd < 0)
        return nullptr;
        
    // Tu tworzymy obiekt TCPClientTransport, który już ma metody send/recv
    return make_unique<TCPClientTransport>(clifd);
}

void TCPServerTransport::close_connection(){
    if(listenfd >= 0){
        close(listenfd);
        listenfd = -1;
    }
}