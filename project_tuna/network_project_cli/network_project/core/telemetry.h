#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <iostream>
#include <memory>		//where unique_ptr resides
#include <vector>

using std::unique_ptr;

class Telemetry{
    public:
    double rtt_ms;
    double jitter;
    int total_packets;
    int mtu;
    int packet_loss;
    /*unique_ptr<Telemetry> telemetry;

    public:
    Telemetry(unique_ptr<Telemetry> telemetry);
    virtual ~Telemetry();
    virtual void update_rtt(double rtt_val) = 0;
    double get_rtt(void) const {return rtt_ms;}
    double get_jitter(void) const {return jitter;}*/
};

#endif
