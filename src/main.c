#include <stdio.h>
#include <stdint.h>

typedef struct {
	const char* ID;
	uint16_t C;
	uint16_t T;
} PeriodicTask;

typedef struct {
	const char* ID;
	uint16_t C;
	uint16_t r;
} AperiodicTask;

typedef struct {
	FILE* fout;
	uint16_t time;
	
	uint8_t pCount;
	uint8_t aCount;
	
	PeriodicTask* pTasks;
	AperiodicTask* aTasks;
} Simulation;

void Simulate(Simulation* plan) {
	
}

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	Simulation* plan = calloc(sizeof(Simulation), 1);
	
	FILE* fin = fopen(filein, "r");
	// Parse the file to get pCount
	// Parse the file to get time
	
	plan->pTasks = calloc(sizeof(PeriodicTask), plan->pCount);
	// Parse the file pCount times to get the data for each periodic task
	
	// Parse the file to get aCount
	plan->aTasks = calloc(sizeof(AperiodicTask), plan->aCount);
	// Parse the file aCount times to get the data for each periodic task
	
	fclose(fin);
	
	
	FILE* fout = fopen(fileout, "w");
	Simulate(periodicTasks, aperiodicTasks, fout);
	fclose(fout);
	
	free(plan->pTasks);
	free(plan->aTasks);
	free(plan);
	
	return 0;
}
