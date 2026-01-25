#include "SCTPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/sctp.h>   // SCTP Library
#include <cmath>        // fabs
#include <cstring>
#include <sodium.h>
#include <iostream>     // Added for cout/cerr

using namespace std;

// Encryption Key (ChaCha20)
static uint8_t CHACHA20_KEY[crypto_stream_chacha20_KEYBYTES] = {
    0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
}; 

SCTPClientTransport::SCTPClientTransport() : sockfd(-1) { 
  stats.rtt_ms = 0.0;
  stats.jitter = 0.0;

  if (sodium_init() < 0) {
      throw std::runtime_error("libsodium error ");
  }
}

SCTPClientTransport::SCTPClientTransport(int existing_fd) : sockfd(existing_fd) {
  if (sodium_init() < 0) {
     throw std::runtime_error("libsodium error ");
  }
}

// Encryption Logic (ChaCha20)
std::vector<uint8_t> SCTPClientTransport::encrypt(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out(data.size());
    uint8_t nonce[crypto_stream_chacha20_NONCEBYTES];
    
    randombytes_buf(nonce, sizeof(nonce));

    // ciphering with xor
    crypto_stream_chacha20_xor(out.data(), data.data(), data.size(), nonce, CHACHA20_KEY);

    std::vector<uint8_t> result(sizeof(nonce) + out.size());
    std::memcpy(result.data(), nonce, sizeof(nonce));
    std::memcpy(result.data() + sizeof(nonce), out.data(), out.size());

    return result;
}

// Decryption Logic (ChaCha20)
std::vector<uint8_t > SCTPClientTransport::decrypt(const std::vector<uint8_t>& data) {
    // check if it can contain nonce
    if(data.size() <  crypto_stream_chacha20_NONCEBYTES)
        throw std::runtime_error("text too short");

    const uint8_t* nonce = data.data( );
    const uint8_t* ciphertext = data.data() +  crypto_stream_chacha20_NONCEBYTES;
    size_t ciphertext_len = data.size()  - crypto_stream_chacha20_NONCEBYTES;

    std::vector<uint8_t> out(ciphertext_len);
    //deciphering
    crypto_stream_chacha20_xor(out.data(), ciphertext, ciphertext_len, nonce, CHACHA20_KEY );

    return out;
}

SCTPClientTransport::~SCTPClientTransport() { close_connection(); }

void SCTPClientTransport::update_mtu(){ 
        struct sctp_status status{};
        socklen_t len = sizeof(status);

        if(getsockopt(sockfd, IPPROTO_SCTP, SCTP_STATUS, &status, &len) == 0) 
                stats.mtu = status.sstat_primary.spinfo_mtu;
}

bool SCTPClientTransport::connectTo(const string& addr, uint16_t port) {
    // SCTP socket creation (One-to-One style)
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) { perror("SCTP Socket error"); return false; }

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &servaddr.sin_addr) <= 0) return false;

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("SCTP Connect error");
        return false;
    }

    // Fix suggested by Gemini 3 Pro
    
    // --- CRITICAL FIX: DISABLE NAGLE ALGORITHM ---
    // This removes the ~200-400ms delay per packet for small messages
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_SCTP, SCTP_NODELAY, &enable, sizeof(enable)) < 0) {
        perror("[SCTP] Warning: Could not set SCTP_NODELAY");
    }

    return true;
}

ssize_t SCTPClientTransport::send(const vector<uint8_t>& data) {
    // Encrypt data before sending
    vector<uint8_t> encrypted = encrypt(data);
    
    this->telemetry_update();
    this->update_mtu();
    
    // Send frame length first (Protocol requirement for stream sockets)
    uint32_t frame_len = htonl(encrypted.size());
    if(::send(sockfd, &frame_len, sizeof(frame_len), 0) <= 0) return -1;

    // Sending encrypted message
    size_t total = 0;
    while(total < encrypted.size( )){ // loop that works when getting the message
        ssize_t sent = ::send(sockfd, encrypted.data() + total, encrypted.size() - total, 0);
        if(sent <= 0) return sent;// error handling
        total += sent; //counter update
    }
    stats.total_bytes_sent += (encrypted.size() + sizeof(frame_len)); 
    stats.total_packets_sent++; 
    return data.size();
}
      
ssize_t SCTPClientTransport::recieve( vector<uint8_t>& data) {
    uint32_t frame_len_net = 0;
    
    // 4 bytes of information regarding next frame (Length Header)
    size_t header_recvd = 0;
    while(header_recvd < sizeof(frame_len_net)) {
        ssize_t r = ::recv(sockfd, ((uint8_t*)&frame_len_net) +  header_recvd, sizeof(frame_len_net ) - header_recvd, 0);
        if(r <= 0) return r;
        header_recvd += r;
    }

    uint32_t frame_len = ntohl(frame_len_net);
    if(frame_len > 65536 * 2) return -1; 

    // Getting cyphered data
    data.resize(frame_len);
    size_t total = 0;
    while(total < frame_len){
        ssize_t r = ::recv(sockfd, data.data() + total, frame_len - total,  0);
        if(r <= 0) return r;
        total += r;
    }

    // Deciphering attempt
    try {
        data = decrypt(data);
        this->stats.total_packets_received++; 
        return data.size();
    } catch (const std::exception& err) {
        std::cerr << " SCTP decryption errorr " << err.what() << std::endl;
        return -1;
    }
}

void SCTPClientTransport::close_connection() {
    if (sockfd >= 0) { ::close(sockfd); sockfd = -1; }
}

void SCTPClientTransport::update_rtt_value(double rtt_val){
    if(stats.total_packets > 0){
        stats.jitter = fabs(rtt_val - stats.rtt_ms);    //new - old rtt, jitter measurement
                                //because its strictly tied to rtt
    }
    stats.rtt_ms = rtt_val; //"new" is our old now

    stats.total_packets++;  //incrementing the packet count
}

void SCTPClientTransport::telemetry_update() {
  auto now = std::chrono::steady_clock::now();
  
  struct sctp_assoc_stats assoc_stat;
  socklen_t assoc_len = sizeof(assoc_stat);

  // Getting kernel stats specific to SCTP association
  if(getsockopt(this->sockfd, IPPROTO_SCTP, SCTP_GET_ASSOC_STATS, &assoc_stat, &assoc_len) == 0) {

  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stats.last_check_time).count();

  if (duration_ms >= 100) {
     double seconds = duration_ms / 1000.0;
  
  // In SCTP we ask OS about packet loss (retransmitted chunks)
  if(this->stats.total_packets > 0){
    // Calculate loss based on retransmissions vs total packets
    this->stats.packet_loss = (double)assoc_stat.sas_rtxchunks / (this->stats.total_packets + assoc_stat.sas_rtxchunks);
  }

    // Throughput calculations (all bytes sent in that particular measurent)
    long long bytes_diff = stats.total_bytes_sent - stats.last_total_bytes;
    this->stats.throughput_kbps = (bytes_diff * 8.0 / 1000.0) / seconds;    //dividing by 1000 because of kilo- (bytes)

    // Goodput calculation (throughput - loss)
    this->stats.goodput_kbps = this->stats.throughput_kbps * (1.0 - this->stats.packet_loss);

    // Everything back to "idle" from the new message perspective
    this->stats.last_check_time = now;
    this->stats.last_total_bytes = stats.total_bytes_sent; // Update last bytes count for next delta
    }   //duration_ms
  }   //getsockopt
}

Telemetry SCTPClientTransport::get_stats(){
    return stats;
}