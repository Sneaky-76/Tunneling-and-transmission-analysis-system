G = g++
GFLAGS = -std=c++17 -Wall
LIBS = -lpthread -lsctp

SRCS = main.cpp \
       ../server_code/server.cpp \
       ../server_code/TCPServerTransport.cpp \
       ../server_code/SCTPServerTransport.cpp \
       ../client_code/TCPClientTransport.cpp \
       ../client_code/SCTPClientTransport.cpp \
       ../client_code/UDPClientTransport.cpp \
       ../client_code/UDPServerTransport.cpp 

TARGET = serverSide

all:
	$(G) $(GFLAGS) $(SRCS) $(LIBS) -o $(TARGET)
clean:
	rm -f $(TARGET)