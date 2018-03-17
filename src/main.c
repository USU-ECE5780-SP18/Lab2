#include "parser.h"
#include "reporter.h"
#include <stdio.h>
#include <stdlib.h>

extern Schedule* RmSimulation(SimPlan* plan);
extern Schedule* EdfSimulation(SimPlan* plan);

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	// Parse the input file
	SimPlan* plan = ParsePlan(filein);
	
	// Run the SimPlan
	Schedule* rmsched = RmSimulation(plan);
	Schedule* edfsched = EdfSimulation(plan);
	
	// Output the results
	FILE* fout = fopen(fileout, "w");
	
	fprintf(fout, "------------------ Rate Monotonic -----------------\r\n");
	WriteSchedule(fout, rmsched);
	fprintf(fout, "\r\n");
	
	fprintf(fout, "------------- Earliest Deadline First -------------\r\n");
	WriteSchedule(fout, edfsched);
	fclose(fout);
	
	// Cleanup
	CleanSchedule(rmsched);
	CleanSchedule(edfsched);
	CleanPlan(plan);
	
	return 0;
}
