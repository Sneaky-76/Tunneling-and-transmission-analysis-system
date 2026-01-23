#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <netinet/in.h>

/*
 * Class responsible for generating background UDP traffic.
 * It runs in a separate thread to simulate network congestion.
 */
class TrafficGenerator {
private:
    int sockfd;
    struct sockaddr_in servaddr;
    std::atomic<bool> running; // Atomic flag for thread safety
    std::thread worker_thread;

    // Internal loop function executed by the thread
    void sending_loop(int packet_size, int interval_ms);

public:
    TrafficGenerator();
    ~TrafficGenerator();

    // Sets up the target IP address and port (e.g., port 9999)
    bool setup(const std::string& ip_addr, uint16_t port);

    // Starts the traffic generation thread
    void start(int packet_size, int interval_ms);

    // Stops the thread and releases resources
    void stop();
};

#endif