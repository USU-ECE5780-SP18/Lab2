#include "parser.h"
#include "reporter.h"
#include <stdio.h>
#include <stdlib.h>

extern Schedule* RmSimulation(SimPlan* plan);
extern Schedule* EdfSimulation(SimPlan* plan);

Schedule* TestSchedule(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);
	int t = 0;

	for (int i = 0; i < plan->tasks; ++i, ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_RELEASED;
	}
	for (int i = 0; i < plan->tasks; ++i, ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_PREEMPTED;
	}
	for (int i = 0; i < plan->tasks; ++i, ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_OVERDUE;
	}
	for (int i = 0; i < plan->tasks; ++t) {
		sched->activeTask[t] = ++i;
	}

	for (int i = 0; i < plan->tasks; ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_RELEASED;
		sched->activeTask[t] = ++i;
	}
	for (int i = 0; i < plan->tasks; ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_PREEMPTED;
		sched->activeTask[t] = ++i;
	}
	for (int i = 0; i < plan->tasks; ++t) {
		sched->flags[(t * plan->tasks) + i] = STATUS_OVERDUE;
		sched->activeTask[t] = ++i;
	}

	return sched;
}

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	// Parse the input file
	SimPlan* plan = ParsePlan(filein);
	
	// Run the SimPlan
	Schedule* sched = TestSchedule(plan);
	Schedule* rmsched = RmSimulation(plan);
	Schedule* edfsched = EdfSimulation(plan);
	
	// Output the results
	FILE* fout = fopen(fileout, "w");

	fprintf(fout, "------------------ Test Schedule ------------------\r\n");
	WriteSchedule(fout, sched);
	fprintf(fout, "\r\n");

	fprintf(fout, "------------------ Rate Monotonic -----------------\r\n");
	WriteSchedule(fout, rmsched);
	fprintf(fout, "\r\n");
	
	fprintf(fout, "------------- Earliest Deadline First -------------\r\n");
	WriteSchedule(fout, edfsched);
	fclose(fout);
	
	// Cleanup
	CleanSchedule(sched);
	CleanSchedule(rmsched);
	CleanSchedule(edfsched);
	CleanPlan(plan);
	
	return 0;
}
