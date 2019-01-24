CC = gcc
CFLAGS = -Wall -g -lpthread

all: server_n_client

server_n_client: bankingServer.o bankingClient.o bankingInterface.o
	$(CC) $(CFLAGS)  -o bankingServer bankingServer.o bankingInterface.o
	$(CC) $(CFLAGS)  -o bankingClient bankingClient.o bankingInterface.o

bankingServer.o: bankingServer.c
	$(CC) $(CFLAGS) -c bankingServer.c

bankingClient.o: bankingClient.c
	$(CC) $(CFLAGS) -c bankingClient.c

bankingInterface.o: bankingInterface.c bankingInterface.h
	$(CC) $(CFLAGS) -c bankingInterface.c

clean:
	rm -rf *.o bankingClient bankingServer

