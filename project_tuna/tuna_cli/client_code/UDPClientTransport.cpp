#include "UDPClientTransport.h"
#include <sys/socket.h>   //core POSIX socekt API
#include <arpa/inet.h>    //inet_pton, so string -> binary conversion
#include <unistd.h>   //POSIX system calls header
#include <cstdio>   //perror
#include <string>
#include <sys/ioctl.h>    //mtu
#include <net/if.h>
#include <cstring>
#include <cmath>
using std::string;
using std::vector;

std::vector<uint8_t> UDPClientTransport::encrypt(const std::vector<uint8_t>& data) {return data;};
std::vector<uint8_t> UDPClientTransport::decrypt(const std::vector<uint8_t>& data) {return data;};

UDPClientTransport::UDPClientTransport() : sockfd(-1) {//stats.rtt_ms = 0.0, stats.jitter = 0.0;}
  this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(this->sockfd >= 0){
  struct timeval tv;
  tv.tv_sec = 3;                      //starting value (Increased to 3s for login stability)
  tv.tv_usec = 0;                //timeout in us [microseconds] for packet loss 
                                      //(packet not registered in time? -> do not increment received)
  setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  this->total_sent_requests=0;        //default values that are needed
  this->total_received_responses=0;
  this->stats.packet_loss=0.0;
}

  stats.rtt_ms = 0.0;                 //again, default values
  stats.jitter = 0.0;
  stats.last_check_time = std::chrono::steady_clock::now();
}
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

    // CRITICAL FIX: We must connect the UDP socket to use send/recv interface consistently
    if (connect(this->sockfd, (struct sockaddr*)&this->destination_addr, sizeof(this->destination_addr)) < 0) {
        perror("Connect error:");
        return 0;
    }

    return 1;
}
    
    ssize_t UDPClientTransport::send(const vector<uint8_t> &data) {
    
      this->total_sent_requests++; //count attempts
      this->update_mtu();
      
    // Use send() because we are connected now
    ssize_t sent = ::send(this->sockfd, data.data(), data.size(), 0);
    
    if (sent > 0) {
        this->stats.total_bytes_sent += sent;
    }
    //this->telemetry_update();
    return sent;
}

    ssize_t UDPClientTransport::recieve(vector<uint8_t> &data) {  
      //socklen_t length = sizeof(source_addr); // Not needed for connected socket
      data.resize(1500);

      // CRITICAL FIX: Removed MSG_DONTWAIT so it waits for the timeout period
      ssize_t recvdBytes = ::recv(this->sockfd, data.data(), data.size(), MSG_DONTWAIT); 

      if(recvdBytes > 0) {
        data.resize(recvdBytes);
        this->total_received_responses++;
      } else {
        data.clear();
        return -1;
      }
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
    stats.jitter = fabs(rtt_val - stats.rtt_ms);  //new - old rtt, jitter measurement
                //because its strictly tied to rtt
  }
        stats.rtt_ms = rtt_val; //"new" is our old now

  stats.total_packets++;  //incrementing the packet count
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
      //Note: We are already connected in connectTo, but this logic preserves your MTU probing style
      if (connect(this->sockfd, (struct sockaddr*)&this->destination_addr, sizeof(this->destination_addr)) == 0) {

        //probing path MTU from the kernel
        if (getsockopt(this->sockfd, IPPROTO_IP, IP_MTU, &mtu, &vallen) == 0 && mtu > 0) {
          this->stats.mtu = mtu;
        } 

        //dissolve association (Not needed if we want to keep connection, but keeping for style 1:1)
        // struct sockaddr_in dissolve = {};
        // dissolve.sin_family = AF_UNSPEC;
        // connect(this->sockfd, (struct sockaddr*)&dissolve, sizeof(dissolve));
      }

      //is still 0 or something failed, just use the default value
      if (this->stats.mtu <= 0) {
        this->stats.mtu = 1500;
      }
    }

void UDPClientTransport::telemetry_update() {
  auto now = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

  //packet loss in time window
  long long sent_in_window = this->total_sent_requests - this->last_sent_count;
  long long recvd_in_window = this->total_received_responses - this->last_recvd_count;
  
  //packet loss in broader time intervals (all "measurement session", so basically one connection to server)
  long long sent_long_term = this->total_sent_requests;
  long long recvd_long_term = this->total_received_responses;

  if (duration_ms >= 100) {
     double seconds = duration_ms / 1000.0;

  //loss caluclation for ONE window only
  if (sent_in_window > 0) {
    this->stats.packet_loss_windowed = 1.0 - (static_cast<double>(recvd_in_window) / static_cast<double>(sent_in_window));
  }
  
  if (sent_long_term >0 ){
    this->stats.packet_loss = 1.0 - (static_cast<double>(recvd_long_term) / static_cast<double>(sent_long_term));
  }

    //throughput calulations  (all bytes sent in that particular measurent)
    long long bytes_diff = stats.total_bytes_sent - stats.last_total_bytes;
    this->stats.throughput_kbps = (bytes_diff * 8.0 / 1000.0) / seconds;    //dividing by 1000 because of kilo- (bytes)

    //goodput calculattion  (throughput - loss)
    this->stats.goodput_kbps = this->stats.throughput_kbps * (1.0- this->stats.packet_loss);

    //everything back to "idle" from the new message perspective
    this->stats.last_check_time = now;
}
}

    Telemetry UDPClientTransport::get_stats(){
        this->stats.total_packets_sent = this->total_sent_requests;
        this->stats.total_packets_received = this->total_received_responses;
  return stats;
    }
