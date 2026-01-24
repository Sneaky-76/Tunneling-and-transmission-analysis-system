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
    
    // Allow immediate restart of the server
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
    
    // Accept the incoming connection
    int clifd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
    if(clifd < 0) return nullptr;
    
    // --- FIX: DISABLE NAGLE ALGORITHM ON SERVER SIDE ---
    // Ensure the server replies immediately  without delay
    int enable = 1;
    if (setsockopt(clifd, IPPROTO_SCTP, SCTP_NODELAY, &enable, sizeof(enable)) < 0) {
        perror("SCTP Server NODELAY error");
        // We continue even if this fails, but it's good to know
    }
    // ---------------------------------------------------

    return make_unique<SCTPClientTransport>(clifd);
}

void SCTPServerTransport::close_connection(){
    if(listenfd >= 0) { close(listenfd); listenfd = -1; }
}