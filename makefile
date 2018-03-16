lab2.exe: main.c reporter.o
	g++ main.c reporter.o -lpthread -g -O0 -o lab2.exe

reporter.o: reporter.c repoter.h
	g++ reporter.c -lpthread -g -O0 -c -o reporter.o

clean:
	rm *.o
	rm lab2.exe
