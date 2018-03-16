lab2: bin/main.o bin/reporter.o
	g++ bin/main.o bin/reporter.o -g -O0 -o lab2

bin/main.o: bin src/main.c
	g++ src/main.c -lpthread -g -O0 -c -o bin/main.o

bin/reporter.o: bin src/reporter.c src/reporter.h
	g++ src/reporter.c -lpthread -g -O0 -c -o bin/reporter.o

bin:
	mkdir -p bin

clean:
	rm bin/*.o
	rm lab2
