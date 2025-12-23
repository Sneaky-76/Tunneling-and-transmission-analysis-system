G = g++
GFLAGS = -std=c++17 -Wall
LIBS = -lpthread

SRCS =	main.cpp \
	../server_code/server.cpp \
	../server_code/TCPServerTransport.cpp \
	../client_code/TCPClientTransport.cpp

TARGET = serverSide

all:
	$(G) $(GFLAGS) $(SRCS) $(LIBS) -o $(TARGET)
