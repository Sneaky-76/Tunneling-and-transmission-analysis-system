#include "TCPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <cmath>		//fabs
//#include <chrono>               //for telemtery update
#include <cstring>
#include <sodium.h>

using namespace std;
// Static 256-bit (32 bytes) symmetric keyy
static uint8_t CHACHA20_KEY[crypto_stream_chacha20_KEYBYTES] = {
    0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
};

TCPClientTransport::TCPClientTransport() : sockfd(-1) { 
stats.rtt_ms = 0.0,
stats.jitter = 0.0;
// initialize sodium
if (sodium_init() < 0) {
        throw std::runtime_error("libsodium error ");
    }
}
TCPClientTransport::TCPClientTransport(int existing_fd) : sockfd(existing_fd) {
// initialize sodium
	if (sodium_init() < 0) {
        throw std::runtime_error("libsodium error ");
    }
}
TCPClientTransport::~TCPClientTransport() { close_connection(); }

void TCPClientTransport::update_mtu(){
        int mtu_val=0;
        socklen_t mtu_length = sizeof(mtu_val);
        if(getsockopt(sockfd, IPPROTO_IP, IP_MTU, &mtu_val, &mtu_length) == 0){
                stats.mtu = mtu_val;
        }else{
                perror("Obtaining MTU have failed.");
        }
}  
// Encryption Logic (ChaCha20)
std::vector<uint8_t> TCPClientTransport::encrypt(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(data.size());
    uint8_t nonce[crypto_stream_chacha20_NONCEBYTES];
    
    // generate a random Nonce
    randombytes_buf(nonce, sizeof(nonce));

    // ciphering with xor
    crypto_stream_chacha20_xor(out.data(), data.data(), data.size(), nonce, CHACHA20_KEY);
    // putting the message together using memcpy: nonce + cipher text
    std::vector<uint8_t> result(sizeof(nonce) + out.size());
    std::memcpy(result.data(), nonce, sizeof(nonce));
    std::memcpy(result.data() + sizeof(nonce), out.data(), out.size());

    return result;
}
// Decryption Logic (ChaCha20)
std::vector<uint8_t > TCPClientTransport::decrypt(const std::vector<uint8_t>& data) {
    // check if it can contain nonce
    if(data.size() <  crypto_stream_chacha20_NONCEBYTES)
        throw std::runtime_error("text too short");
    // read nonce from the begining of message
    const uint8_t*  nonce = data.data( );
    const uint8_t* ciphertext = data.data() +  crypto_stream_chacha20_NONCEBYTES;
    size_t ciphertext_len = data.size()  - crypto_stream_chacha20_NONCEBYTES;

    std::vector<uint8_t> out(ciphertext_len);
    // Read Nonce from the beginning of the received packet.
    crypto_stream_chacha20_xor(out.data(), ciphertext, ciphertext_len, nonce, CHACHA20_KEY );

    return out;
}

bool TCPClientTransport::connectTo(const string& addr, uint16_t port){
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Socket error: ");
		return 0;
	}

	struct sockaddr_in servaddr{};
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0){	//c_str provides a pointer
									//to the first char
		//inet_pton: 0 if src not found, -1 if error. warning about both
		perror("Pton error: ");
		return 0;
	}

	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){ //0 = success, -1 = error
		perror("Connect error: ");
		return 0;
	}

	//removing Nagle’s algorithm (no additional waiting while sending small packets)
	int enable = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) < 0) {
        perror("[TCP] Warning: Could not set TCP_NODELAY");
    }
	
	return 1;
}

ssize_t TCPClientTransport::send(const vector<uint8_t>& data){
      
        update_mtu();
        telemetry_update();
        vector<uint8_t> encrypted = encrypt(data);
    // Send frame length first (Protocol requirement for stream sockets)
        uint32_t frame_len = htonl(encrypted.size());
        size_t header_sent = 0;
        while(header_sent < sizeof(frame_len)){
                ssize_t snd = ::write(sockfd, ((uint8_t*)&frame_len) + header_sent,sizeof(frame_len) - header_sent);
                if(snd <= 0) return snd;
                        header_sent += snd;
        }
        size_t total = 0;
        while(total < encrypted.size()){// loop that works when getting the message
                ssize_t snd = ::write(sockfd, encrypted.data() + total,
                                encrypted.size() - total);
                if(snd <= 0) return snd;// error handling
                total += snd;//counter update
        }


       
        stats.total_bytes_sent += (encrypted.size() + sizeof(frame_len)); // CHECK
        stats.total_packets_sent++; //should work but not sure
        return data.size(); 
}

ssize_t TCPClientTransport::recieve(vector<uint8_t>& data){
    // 4 bytes of information regarding next frame (Length Header)
    uint32_t frame_len_net = 0;
    size_t header_received = 0;
    
    while(header_received < sizeof(frame_len_net)){
        ssize_t read = ::read(sockfd, ((uint8_t*)&frame_len_net) + header_received,
                              sizeof(frame_len_net) - header_received);
        if(read <= 0) return read;
        header_received += read;
    }
    
    uint32_t frame_len = ntohl(frame_len_net);
    
    if(frame_len > 65536 * 2){ // Zlimit higher bcs of nonce
        return -1;
    }
    
    data.resize(frame_len);
    size_t total = 0;
    while(total < frame_len){
        ssize_t read = ::read(sockfd, data.data() + total,
                              frame_len - total);
        if(read <= 0) return read;
        total += read;
    }
    // Deciphering attempt, if there is problem exception is thrown
    try {
        data = decrypt(data);

        stats.total_packets_received++; // eventually change the place for this line CHECK

        return data.size();
    } catch (const std::exception& e) {
        
        std::cerr << "error: decryption failed: " << e.what() << std::endl;
        return -1;
    }
}


void TCPClientTransport::close_connection(){
	if(sockfd >= 0){	//checking if the socket is stil open
		::close(sockfd);
		sockfd = -1; 	//prevention from closing multiple times
	}
}

void TCPClientTransport::update_rtt_value(double rtt_val){
	if(stats.total_packets > 0){
		stats.jitter = fabs(rtt_val - stats.rtt_ms); //new - old rtt
	}
	stats.rtt_ms = rtt_val; //"new" is our old now

	stats.total_packets++;	//incrementing the packet count
}

void TCPClientTransport::telemetry_update() {
  auto now = std::chrono::steady_clock::now();
  
  struct tcp_info info;
  socklen_t info_len = sizeof(info);

  //getting kernel data
  if (getsockopt(this->sockfd, IPPROTO_TCP, TCP_INFO, &info, &info_len) == 0) {

  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

  if (duration_ms >= 100) {
     double seconds = duration_ms / 1000.0;
  
  //in TCP we ask OS about packet loss
  if(this->stats.total_packets){  //are there packets? if so, then count the loss | _
        this->stats.packet_loss = (double)info.tcpi_total_retrans/(this->stats.total_packets +info.tcpi_total_retrans);
  }

    //throughput calulations  (all bytes sent in that particular measurent)
    long long bytes_diff = stats.total_bytes_sent - stats.last_total_bytes;
    this->stats.throughput_kbps = (bytes_diff * 8.0 / 1000.0) / seconds;    //dividing by 1000 because of kilo- (bytes)

    //goodput calculattion  (throughput - loss)
    this->stats.goodput_kbps = this->stats.throughput_kbps * (1.0 - this->stats.packet_loss);

    //everything back to "idle" from the new message perspective
    this->stats.last_check_time = now;
    }   //duration_ms
}   //getsockopt

}

    Telemetry TCPClientTransport::get_stats(){
        //this->stats.total_packets_sent = this->total_sent_requests;
        //this->stats.total_packets_received = this->total_received_responses;
	return stats;
    }


