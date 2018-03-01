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

void Simulate(PeriodicTask* periodicTasks, AperiodicTask* aperiodicTasks, FILE* fout) {
	
}

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	FILE* fin = fopen(filein, "r");
	// Parse the file to get n and contents of each periodic task
	PeriodicTask* periodicTasks = calloc(sizeof(PeriodicTask), n);
	AperiodicTask* aperiodicTasks = calloc(sizeof(AperiodicTask), n);
	// periodicTasks[i]
	fclose(fin);
	
	
	FILE* fout = fopen(fileout, "w");
	Simulate(periodicTasks, aperiodicTasks, fout);
	fclose(fout);
	
	return 0;
}
