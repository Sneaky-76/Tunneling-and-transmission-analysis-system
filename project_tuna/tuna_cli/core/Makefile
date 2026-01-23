G = g++
GFLAGS = -std=c++17 -Wall
LIBS = -lpthread -lsctp -lsodium

SRCS = main.cpp \
       ../client_code/client.cpp \
       ../client_code/TCPClientTransport.cpp \
       ../client_code/SCTPClientTransport.cpp \
       ../client_code/TrafficGenerator.cpp \
       ../client_code/UDPClientTransport.cpp 
       
TARGET = clientSide

all:
	$(G) $(GFLAGS) $(SRCS) $(LIBS) -o $(TARGET)
clean:
	rm -f $(TARGET)