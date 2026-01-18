#include "TrafficGenerator.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>

TrafficGenerator::TrafficGenerator() : sockfd(-1), running(false) {}

TrafficGenerator::~TrafficGenerator() {
    stop();
    if (sockfd >= 0) {
        close(sockfd);
    }
}

bool TrafficGenerator::setup(const std::string& ip_addr, uint16_t port) {
    // Creation of a UDP socket (SOCK_DGRAM)
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[Background] Socket creation failed");
        return false;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    
    // IP address conversion from text to binary form
    if (inet_pton(AF_INET, ip_addr.c_str(), &servaddr.sin_addr) <= 0) {
        perror("[Background] Invalid address provided");
        return false;
    }
    return true;
}

void TrafficGenerator::start(int packet_size, int interval_ms) {
    if (running) return; // Prevention of multiple starts
    running = true;
    
    // Launching the worker thread
    worker_thread = std::thread(&TrafficGenerator::sending_loop, this, packet_size, interval_ms);
}

void TrafficGenerator::stop() {
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join(); // Waiting for the thread to finish execution
    }
}

void TrafficGenerator::sending_loop(int packet_size, int interval_ms) {
    // Initialization of dummy data vector
    std::vector<uint8_t> junk(packet_size, 'X'); 

    std::cout << "[Background] Traffic generator started (Target port: " 
              << ntohs(servaddr.sin_port) << ")..." << std::endl;

    while (running) {
        // Sending UDP packet to the target
        sendto(sockfd, junk.data(), junk.size(), 0, 
               (const struct sockaddr*)&servaddr, sizeof(servaddr));

        // Optional delay to control bandwidth usage
        if (interval_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    }
    std::cout << "[Background] Traffic generator stopped." << std::endl;
}