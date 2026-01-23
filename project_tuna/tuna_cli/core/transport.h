#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "telemetry.h"

using namespace std;
//using std::string, std::int, std::vector; cant use it like that for some reason

class Transport{
//probably better tu put "Telemetry stats" here under protected
public:
	virtual ~Transport() = default;
	virtual bool connectTo(const string& addr, uint16_t port) = 0;
	virtual ssize_t send(const vector<uint8_t>& data) = 0;
	virtual ssize_t recieve(vector<uint8_t>& data) = 0;
	virtual void close_connection() = 0;

	virtual void update_rtt_value(double rtt_val) = 0;
	virtual void update_mtu() = 0;
	virtual Telemetry get_stats() = 0;
	
	virtual void telemetry_update()=0;
	virtual std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data) = 0;
	virtual std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data) = 0;
};

#endif
