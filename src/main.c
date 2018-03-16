#include "parser.h"
#include "reporter.h"
#include <stdio.h>

void Simulate(SimPlan* plan) {
	//fprintf(plan->reporter.fout, "------------- Rate Monotonic --------------\r\n");
	//reportInit(plan);
	//RMSchedule(plan);
	//reportFlushAll(plan);
	//fprintf(plan->reporter.fout, "\r\n");
	
	//fprintf(plan->reporter.fout, "------------- Earliest Deadline First --------------\r\n");
	//reportInit(plan);
	//EDFSchedule(plan);
	//reportFlushAll(plan);
}

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	// Parse the input file
	SimPlan* plan = ParsePlan(filein);
	
	// Run the SimPlan
	//FILE* fout = fopen(fileout, "w");
	//Simulate(plan);
	//fclose(fout);
	
	// Cleanup plan
	CleanPlan(plan);
	
	return 0;
}
