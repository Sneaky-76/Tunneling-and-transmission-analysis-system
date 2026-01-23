#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <iostream>
#include <memory>		//where unique_ptr resides
#include <vector>
#include <chrono>

using std::unique_ptr;

class Telemetry{
    public:
    double rtt_ms=0;
    double jitter=0;
    int total_packets=0;
    int mtu;
    //unique_ptr<Telemetry> telemetry;
    
    double throughput_kbps=0.0;
    double goodput_kbps=0.0;
    long long last_total_bytes=0;
    long long total_bytes_sent=0;
    std::chrono::steady_clock::time_point last_check_time = std::chrono::steady_clock::now();

    //variables for packet loss
    //uint32_t last_packet_sequence=0;    //probably gonna be used for UDP
    double packet_loss=0.0;
    double packet_loss_windowed=0.0;      //windowed version for particular measurement
    int total_packets_sent=0;
    int total_packets_received=0;
  
};

#endif
