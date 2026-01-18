#ifndef TRAFFIC_SINK_H
#define TRAFFIC_SINK_H

#include <thread>
#include <atomic>
#include <netinet/in.h>

/*
 * Class responsible for sinking (receiving and discarding) background UDP traffic.
 * Prevents ICMP "Port Unreachable" responses from the OS.
 */
class TrafficSink {
private:
    int sockfd;
    std::atomic<bool> running;
    std::thread sink_thread;

    // Internal loop function for receiving data
    void receive_loop();

public:
    TrafficSink();
    ~TrafficSink();

    // Opens a UDP socket on the specified port and starts the thread
    bool start(uint16_t port);
    
    // Stops the receiver thread and closes the socket
    void stop();
};

#endif