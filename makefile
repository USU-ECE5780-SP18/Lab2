lab2: bin/main.o bin/reporter.o bin/parser.o bin/rmsched.o bin/edfsched.o
	g++ bin/main.o bin/reporter.o bin/parser.o bin/rmsched.o bin/edfsched.o -g -O0 -o lab2

bin/main.o: bin src/main.c src/parser.h src/reporter.h
	g++ src/main.c -lpthread -g -O0 -c -o bin/main.o

bin/parser.o: bin src/parser.c src/parser.h
	g++ src/parser.c -lpthread -g -O0 -c -o bin/parser.o

bin/reporter.o: bin src/reporter.c src/reporter.h
	g++ src/reporter.c -lpthread -g -O0 -c -o bin/reporter.o

bin/rmsched.o: bin src/rmsched.c src/parser.h src/reporter.h src/sched.h
	g++ src/rmsched.c -lpthread -g -O0 -c -o bin/rmsched.o

bin/edfsched.o: bin src/edfsched.c src/parser.h src/reporter.h src/sched.h
	g++ src/edfsched.c -lpthread -g -O0 -c -o bin/edfsched.o

bin:
	mkdir -p bin

clean:
	rm bin/*.o
	rm lab2
