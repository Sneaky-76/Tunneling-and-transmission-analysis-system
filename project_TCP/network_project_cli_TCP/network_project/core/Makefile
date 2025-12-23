G = g++
GFLAGS = -std=c++17 -Wall
LIBS = -lpthread

SRCS =	main.cpp \
	../client_code/client.cpp \
	../client_code/TCPClientTransport.cpp

TARGET = clientSide

all:
	$(G) $(GFLAGS) $(SRCS) $(LIBS) -o $(TARGET)
