#include "UDPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <sys/ioctl.h>    //mtu
#include <net/if.h>
#include <cstring>
#include <cmath>
using std::string;
using std::vector;

UDPClientTransport::UDPClientTransport(): sockfd(-1){ stats.rtt_ms = 0.0, stats.jitter = 0.0;}
UDPClientTransport::~UDPClientTransport(){close_connection();}

bool UDPClientTransport::connectTo(const string &addr, uint16_t port){
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd <0){
        perror("Socket error:");
        return 0;
    }


    //struct sockaddr_in servaddr {};
    this->destination_addr.sin_family = AF_INET;
    this->destination_addr.sin_port = htons(port);

    

    if(inet_pton(AF_INET,addr.c_str(),&this->destination_addr.sin_addr) <= 0){

        perror("Pton error:");
        return 0;
    }

    //if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){ //0 = success, -1 = error
	//	perror("Connect error: ");
	//	return 0;
    //}
    return 1;
}

    ssize_t UDPClientTransport::send(const vector<uint8_t> &data){

        update_mtu();
        return  ::sendto(sockfd, data.data(),data.size(),0,(struct sockaddr*)&destination_addr, sizeof(destination_addr));
    }

    ssize_t UDPClientTransport::recieve(vector<uint8_t> &data){
        socklen_t length = sizeof(source_addr);
        data.resize(512);
        ssize_t recvdBytes =recvfrom(sockfd,data.data(), data.size(),0,(struct sockaddr*)&source_addr, &length);
        if(recvdBytes >0)
            data.resize(recvdBytes);
        return recvdBytes;
    }
    

    void UDPClientTransport::close_connection(){
        if(sockfd >=0){
            close(sockfd);
            sockfd=-1;
        }
    }
    
    void UDPClientTransport::update_rtt_value(double rtt_val){
	if(stats.total_packets > 0){
		stats.jitter = fabs(rtt_val - stats.rtt_ms); 	//new - old rtt, jitter measurement
								//because its strictly tied to rtt
	}
        stats.rtt_ms = rtt_val; //"new" is our old now

	stats.total_packets++;	//incrementing the packet count
    }
    
    void UDPClientTransport::update_mtu() {
      int mtu = 0;
      socklen_t vallen = sizeof(mtu);

      //if we try to connect to a null address
      if (this->destination_addr.sin_addr.s_addr == 0) {
        this->stats.mtu = 1500;
      return;
      }

      //connect briefly to allow getsockopt to work (wont work otherwise)
      if (connect(this->sockfd, (struct sockaddr*)&this->destination_addr, sizeof(this->destination_addr)) == 0) {

        //probing path MTU from the kernel
        if (getsockopt(this->sockfd, IPPROTO_IP, IP_MTU, &mtu, &vallen) == 0 && mtu > 0) {
          this->stats.mtu = mtu;
        } 

        //dissolve association
        struct sockaddr_in dissolve = {};
        dissolve.sin_family = AF_UNSPEC;
        connect(this->sockfd, (struct sockaddr*)&dissolve, sizeof(dissolve));
      }

      //is still 0 or something failed, just use the default value
      if (this->stats.mtu <= 0) {
        this->stats.mtu = 1500;
      }
    }

    Telemetry UDPClientTransport::get_stats(){
	return stats;
    }

