#include <stdio.h>

typedef struct {
	const char* ID;
	int C;
	int T;
} PeriodicTask;

typedef struct {
	const char* ID;
	int C;
	int r;
} AperiodicTask;

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	return 0;
}
