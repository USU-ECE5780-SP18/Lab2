lab5: main.cpp net.hpp bin/chksum.o
	g++ main.c chksum.o -lpthread -g -O0 -o lab5

chksum.o: chksum.c
	g++ chksum.c -c -o bin/chksum.o

clean:
	rm bin/*.o
	rm lab5
