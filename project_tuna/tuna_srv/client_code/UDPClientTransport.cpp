#include "UDPClientTransport.h"
#include <sys/socket.h>		//core POSIX socekt API
#include <arpa/inet.h>		//inet_pton, so string -> binary conversion
#include <unistd.h>		//POSIX system calls header
#include <cstdio>		//perror
#include <string>
using std::string;
using std::vector;

UDPClientTransport::UDPClientTransport(): sockfd(-1){}
UDPClientTransport::~UDPClientTransport(){close_connection();}


bool UDPClientTransport::connectTo(const string &addr, uint16_t port){
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd <0){
        perror("Socket error:");
        return 0;
    }


    //struct sockaddr_in servaddr {};
    destination_addr.sin_family = AF_INET;
    destination_addr.sin_port = htons(port);



    if(inet_pton(AF_INET,addr.c_str(),&destination_addr.sin_addr) <= 0){

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

        return ::sendto(sockfd, data.data(),data.size(),0,(struct sockaddr*)&destination_addr, sizeof(destination_addr));
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

