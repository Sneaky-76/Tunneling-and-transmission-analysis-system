#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <iostream>
#include <memory>		//where unique_ptr resides
#include <vector>
#include <chrono>

using std::unique_ptr;  //this one actually won't work -> std::chrono;

class Telemetry{
    public:
    virtual ~Telemetry() = default;

    double rtt_ms=0;
    double jitter=0;
    int total_packets=0;    //what were you used for buddy...
    int mtu=0;

    double throughput_kbps=0.0;
    double goodput_kbps=0.0;
    long long last_total_bytes=0;
    long long total_bytes_sent=0;
    std::chrono::steady_clock::time_point last_check_time = std::chrono::steady_clock::now();

    //variables for packet loss
    //uint32_t last_packet_sequence=0;    //probably gonna be used for UDP
    int packet_loss=0;
    int total_packets_sent=0;
    int total_packets_recieved=0;
    
    double get_loss_percent() {        //no idea where else to put it, double 'cause %
        if (total_packets_sent == 0){
            return 0.0;
        }
        return (static_cast<double>(packet_loss)/total_packets_sent) * 100.0;
    }

    //virtual void UDP_packet_gap_check(uint32_t incoming_sequence);  //check for udp, others with getsocket
    //virtual void TCP_packet_gap_check(int sockfd);
    //virtual void SCTP_packet_gap_check(int sockfd);
};

#endif
