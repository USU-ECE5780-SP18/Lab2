lab2: bin/main.o bin/reporter.o bin/parser.o bin/rmsched.o bin/edfsched.o
	mkdir -p bin
	g++ bin/main.o bin/reporter.o bin/parser.o bin/rmsched.o bin/edfsched.o -g -O0 -o lab2

bin/main.o: src/main.c src/parser.h src/reporter.h
	mkdir -p bin
	g++ src/main.c -g -O0 -c -o bin/main.o

bin/parser.o: src/parser.c src/parser.h
	mkdir -p bin
	g++ src/parser.c -g -O0 -c -o bin/parser.o

bin/reporter.o: src/reporter.c src/reporter.h
	mkdir -p bin
	g++ src/reporter.c -g -O0 -c -o bin/reporter.o

bin/rmsched.o: src/rmsched.c src/parser.h src/reporter.h
	mkdir -p bin
	g++ src/rmsched.c -g -O0 -c -o bin/rmsched.o

bin/edfsched.o: src/edfsched.c src/parser.h src/reporter.h
	mkdir -p bin
	g++ src/edfsched.c -g -O0 -c -o bin/edfsched.o

clean:
	rm bin/*.o
	rm lab2
