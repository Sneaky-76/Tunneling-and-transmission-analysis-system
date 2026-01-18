#include "TrafficSink.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>

TrafficSink::TrafficSink() : sockfd(-1), running(false) {}

TrafficSink::~TrafficSink() {
    stop();
    if(sockfd >= 0) close(sockfd);
}

bool TrafficSink::start(uint16_t port) {
    if(running) return true; // Already running

    // UDP socket creation
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all interfaces
    addr.sin_port = htons(port);

    // Binding the socket to the port
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[Sink] Bind failed");
        return false;
    }

    running = true;
    sink_thread = std::thread(&TrafficSink::receive_loop, this);
    std::cout << "[Sink] Listening for background noise on port " << port << "...\n";
    return true;
}

void TrafficSink::stop() {
    running = false;
    if(sockfd >= 0) {
        // Shutting down the socket to unblock the recv() call
        shutdown(sockfd, SHUT_RDWR); 
        close(sockfd);
        sockfd = -1;
    }
    if(sink_thread.joinable()) {
        sink_thread.join();
    }
}

void TrafficSink::receive_loop() {
    std::vector<uint8_t> buffer(2048);
    while(running) {
        // Data reception and immediate discard (sinkhole behavior)
        ssize_t bytes = recv(sockfd, buffer.data(), buffer.size(), 0);
        
        // Break loop on error or socket closure
        if(bytes <= 0) break; 
    }
}