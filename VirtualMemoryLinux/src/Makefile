CC = gcc
CFLAGS = -Wall -O3

build: libtema-vm.so

libtema-vm.so: tema-mv.o
	gcc -fPIC -shared -o libvm.so $^

tema-vm.o: tema-mv.c

clean:
	rm -f *~ *.o libvm.so __ram __swap
