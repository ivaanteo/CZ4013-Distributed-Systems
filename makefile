CC=g++
CFLAGS=-std=c++11 -Wall

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o bin/server server.cpp

client: client.cpp
	$(CC) $(CFLAGS) -o bin/client client.cpp

clean:
	rm -f server client