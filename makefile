lab2.exe: src/main.c
	g++ src/main.c -lpthread -g -O0 -o lab2.exe

clean:
	rm bin/*.o
	rm lab2.exe
