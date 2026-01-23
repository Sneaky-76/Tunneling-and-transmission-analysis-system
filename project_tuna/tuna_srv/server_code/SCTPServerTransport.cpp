#include "SCTPServerTransport.h"
#include "../client_code/SCTPClientTransport.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <cstdio>

SCTPServerTransport::SCTPServerTransport() : listenfd(-1){}
SCTPServerTransport::~SCTPServerTransport() { close_connection(); }

bool SCTPServerTransport::bindAndListen(uint16_t port){
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if(listenfd < 0) { perror("Socket failed"); return false; }
    
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) return false;
    if(listen(listenfd, 5) < 0) return false;
    return true;
}

unique_ptr<Transport> SCTPServerTransport::acceptClient(){
    struct sockaddr_in cliaddr{};
    socklen_t len = sizeof(cliaddr);
    int clifd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
    if(clifd < 0) return nullptr;
    
    return make_unique<SCTPClientTransport>(clifd);
}

void SCTPServerTransport::close_connection(){
    if(listenfd >= 0) { close(listenfd); listenfd = -1; }
}