CC=gcc

build: server client

server: server.o hash.o
	gcc -O3 server.o hash.o -o server -lrt

client: client.o hash.o
	gcc -O3 client.o hash.o -o client -lrt

server.o: src/server.c
	gcc -O3 -c src/server.c -o server.o

client.o: src/client.c
	gcc -O3 -c src/client.c -o client.o

hash.o: src/hash.c
	gcc -O3 -c src/hash.c -o hash.o

clean:
	rm -rf server client *.o
