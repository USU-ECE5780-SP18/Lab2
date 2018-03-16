lab2.exe: src/main.c reporter.o
	g++ src/main.c reporter.o -lpthread -g -O0 -o lab2.exe

reporter.o: src/reporter.c src/reporter.h
	g++ src/reporter.c -lpthread -g -O0 -c -o reporter.o

clean:
	rm *.o
	rm lab2.exe
