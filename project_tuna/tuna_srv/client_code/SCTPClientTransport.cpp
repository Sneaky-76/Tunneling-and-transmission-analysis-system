#include "SCTPClientTransport.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <netinet/sctp.h> // Biblioteka SCTP
#include <cstring>
#include <sodium.h>
#include <vector>
static uint8_t CHACHA20_KEY[crypto_stream_chacha20_KEYBYTES] = {
    0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
}; 

SCTPClientTransport::SCTPClientTransport() : sockfd(-1) {
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium error ");
    }
}
SCTPClientTransport::SCTPClientTransport(int existing_fd) : sockfd(existing_fd) {
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium error ");
    }
}
SCTPClientTransport::~SCTPClientTransport() { close_connection(); }
// same as tcp
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

std::vector<uint8_t > SCTPClientTransport::decrypt(const std::vector<uint8_t>& data) {
    // check if it can contain nonce
    if(data.size() <  crypto_stream_chacha20_NONCEBYTES)
        throw std::runtime_error("text too short");

    const uint8_t*  nonce = data.data( );
    const uint8_t* ciphertext = data.data() +  crypto_stream_chacha20_NONCEBYTES;
    size_t ciphertext_len = data.size()  - crypto_stream_chacha20_NONCEBYTES;

    std::vector<uint8_t> out(ciphertext_len);
    //deciphering
    crypto_stream_chacha20_xor(out.data(), ciphertext, ciphertext_len, nonce, CHACHA20_KEY );

    return out;
}
bool SCTPClientTransport::connectTo(const string& addr, uint16_t port) {
    // Tworzymy gniazdo SCTP (One-to-One style)
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
    return true;
}

ssize_t SCTPClientTransport::send(const vector<uint8_t>& data) {
    vector<uint8_t> encrypted = encrypt(data);
    
    // length of the message
    uint32_t frame_len = htonl(encrypted.size());
    
    if(::send(sockfd, &frame_len, sizeof(frame_len), 0) <= 0) return -1;

    //sending encrypted message
    size_t total = 0;
    while(total < encrypted.size( )){ // loop that works when getting the message
        ssize_t sent = ::send(sockfd, encrypted.data() + total, encrypted.size() - total, 0);
        if(sent <= 0) return sent;// error handling
        total += sent; //counter update
    }
    return data.size();
}
ssize_t SCTPClientTransport::recieve(vector<uint8_t>& data) {
   uint32_t frame_len_net = 0;
    
    // 4 bytes of information regarding next frame
    size_t header_recvd = 0;
    while(header_recvd < sizeof(frame_len_net)) {
        ssize_t r = ::recv(sockfd, ((uint8_t*)&frame_len_net) +  header_recvd, sizeof(frame_len_net ) - header_recvd, 0);
        if(r <= 0) return r;
        header_recvd += r;
    }

    uint32_t frame_len = ntohl(frame_len_net);
    if(frame_len > 65536 * 2) return -1; 

    // getting cyphered data
    data.resize(frame_len);
    size_t total = 0;
    while(total < frame_len){
        ssize_t r = ::recv(sockfd, data.data() + total, frame_len - total,  0);
        if(r <= 0) return r;
        total += r;
    }
    // deciphering attempt
    try {
        data = decrypt(data);
        return data.size();
    } catch (const std::exception& err) {
        std::cerr << " SCTP decryption errorr " << err.what() << std::endl;
        return -1;
    }
}

void SCTPClientTransport::close_connection() {
    if (sockfd >= 0) { ::close(sockfd); sockfd = -1; }
}