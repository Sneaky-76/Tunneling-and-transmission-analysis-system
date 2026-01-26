#include "TCPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
#include <cstring>
#include <sodium.h>
// Static 256-bit (32 bytes) symmetric keyy
static uint8_t CHACHA20_KEY[crypto_stream_chacha20_KEYBYTES] = {
    0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,
    0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
};

TCPClientTransport::TCPClientTransport() : sockfd(-1) {
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

TCPClientTransport::~TCPClientTransport() { close_connection(); }

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
	return 1;
}

ssize_t TCPClientTransport::send(const vector<uint8_t>& data){    
    //ciphering
    vector<uint8_t> encrypted = encrypt(data);
    // Send frame length first (Protocol requirement for stream sockets)
    uint32_t frame_len = htonl(encrypted.size());
    size_t header_sent = 0;
    while(header_sent < sizeof(frame_len)){
        ssize_t sent = ::send(sockfd, ((uint8_t*)&frame_len) + header_sent,
                             sizeof(frame_len) - header_sent, 0);
        if(sent <= 0) return sent;
        header_sent += sent;
    }
    
    size_t total = 0;
    while(total < encrypted.size()){
        ssize_t sent = ::send(sockfd, encrypted.data() + total,
                             encrypted.size() - total, 0);
        if(sent <= 0) return sent;
        total += sent;
    }
    
    return data.size(); 
}

ssize_t TCPClientTransport::recieve(vector<uint8_t>& data){
    // 4 bytes of information regarding next frame (Length Header)
    uint32_t frame_len_net = 0;
    size_t header_received = 0;
    
    while(header_received < sizeof(frame_len_net)){
        ssize_t recvd = ::recv(sockfd, ((uint8_t*)&frame_len_net) + header_received,
                              sizeof(frame_len_net) - header_received, 0);
        if(recvd <= 0) return recvd;
        header_received += recvd;
    }
    
    uint32_t frame_len = ntohl(frame_len_net);
    
    if(frame_len > 65536 * 2){ // Zlimit higher bcs of nonce
        return -1;
    }
    
    data.resize(frame_len);
    size_t total = 0;
    while(total < frame_len){
        ssize_t recvd = ::recv(sockfd, data.data() + total,
                              frame_len - total, 0);
        if(recvd <= 0) return recvd;
        total += recvd;
    }
    
    try {
        data = decrypt(data);
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

