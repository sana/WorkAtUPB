CC = gcc
CFLAGS = -O3 -Wall

build: client server

client: client.o os_wrapper.o

client.o: client.c

server: server.o os_wrapper.o

server.o: server.c

os_wrapper.o: os_wrapper.c

clean:
	rm -rf server server.o client client.o
