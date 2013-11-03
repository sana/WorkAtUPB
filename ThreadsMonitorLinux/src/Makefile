CC = g++
CFLAGS = -Wall -O3

build: LibMonitor.so LibRW.so

LibMonitor.so: LibMonitor.o
	g++ -fPIC -shared -o LibMonitor.so $^

LibRW.so: LibRW.o
	g++ -fPIC -shared -o LibRW.so $^

LibMonitor.o: LibMonitor.cpp
LibRW.o: LibRW.cpp

clean:
	rm -f *~ *.o *.so
