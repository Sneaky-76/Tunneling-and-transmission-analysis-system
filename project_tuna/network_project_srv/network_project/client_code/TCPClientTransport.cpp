#include "TCPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>

TCPClientTransport::TCPClientTransport() : sockfd(-1) {}
TCPClientTransport::TCPClientTransport(int existing_fd) : sockfd(existing_fd) {}
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
	return ::send(sockfd, data.data(), sizeof(data), 0);	//send requires const void* data, which
                                                                //.data() provides (pointer because of vector
}

ssize_t TCPClientTransport::recieve(vector<uint8_t>& data){
	data.resize(512);	//our max byte count in buffer
	ssize_t recvdBytes = ::recv(sockfd, data.data(), data.size(), 0);
	if(recvdBytes > 0)
		data.resize(recvdBytes);
	return  recvdBytes;
}

void TCPClientTransport::close_connection(){
	if(sockfd >= 0){	//checking if the socket is stil open
		::close(sockfd);
		sockfd = -1; 	//prevention from closing multiple times
	}
}

