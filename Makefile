all: client server
client: client.c
	gcc --std=c99 -o client client.c
server: server.c
	gcc --std=c99 -o server server.c
clean:
	rm -f *.o
	rm -f server
	rm -f client
