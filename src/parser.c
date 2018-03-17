#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The file layout matches our struct layouts for both periodic and aperiodic
// So we can conveniently typecast as either or and parse correctly
inline void ParseTask(char* buff, size_t line_n, PeriodicTask* task) {
	// Get the ID
	int eos = 0; // end of string
	while (eos < line_n && buff[eos] != ',') { ++eos; }
	// A fully rigorous program would probably do some validation here
	// calloc for convenient null terminator
	// performance implications compared to manually adding \0 after malloc and memcpy unknown
	task->ID = (char*)calloc(sizeof(char), eos + 1);
	memcpy(task->ID, buff, eos);

	// Get the execution time
	int bos = eos++; // beginning of string
	while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
	while (eos < line_n && buff[eos] != ',') { ++eos; }
	// A fully rigorous program would probably do some validation here
	buff[eos] = 0; // create null pointer to aid atoi
	task->C = atoi(buff + bos);

	// Get the period (for periodic) or the absolute release time (for aperiodic)
	bos = eos++;
	while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
	while (eos < line_n && buff[eos] != ',') { ++eos; }
	// A fully rigorous program would probably do some validation here
	buff[eos] = 0; // create null pointer to aid atoi
	task->T = atoi(buff + bos); // same memory location and size for (AperiodicTask*)->r
}

SimPlan* ParsePlan(const char* file) {
	SimPlan* plan = (SimPlan*)calloc(sizeof(SimPlan), 1);
	FILE* fin = fopen(file, "r");
	
	size_t buffsize = 128;
	char* buff = (char*)calloc(sizeof(char), buffsize);
	
	// Parse the file to get pCount
	fgets(buff, buffsize, fin);
	plan->pCount = atoi(buff);

	// Parse the file to get time
	fgets(buff, buffsize, fin);
	plan->duration = atoi(buff);
	
	printf("Time: %i\npCount: %i\n", plan->duration, plan->pCount);
	
	plan->pTasks = (PeriodicTask*)calloc(sizeof(PeriodicTask), plan->pCount);
	
	size_t line_n; // n is the sizeof each read line including 2 for \r\n
	size_t id_n; // a counter used to get the size of a task id
	
	// Parse the file pCount times to get the data for each periodic task
	for(int i = 0; i < plan->pCount; ++i) {
		line_n = getline(&buff, &buffsize, fin);
		PeriodicTask* task = (plan->pTasks) + i;
		ParseTask(buff, line_n, task);
		task->rIndex = i + 1;
		printf("pTasks[%i]: {ID: \"%s\", C: %i, T: %i}\n", i, task->ID, task->C, task->T);
	}
	
	// Parse the file to get aCount
	fgets(buff, buffsize, fin);
	plan->aCount = atoi(buff);
	
	printf("aCount: %i\n", plan->aCount);
	
	plan->aTasks = (AperiodicTask*)calloc(sizeof(AperiodicTask), plan->aCount);
	
	// Parse the file aCount times to get the data for each periodic task
	for(int i = 0; i < plan->aCount; ++i) {
		line_n = getline(&buff, &buffsize, fin);
		AperiodicTask* task = (plan->aTasks) + i;
		ParseTask(buff, line_n, (PeriodicTask*)task);
		task->rIndex = plan->pCount + i + 1;
		printf("aTasks[%i]: {ID: \"%s\", C: %i, r: %i}\n", i, task->ID, task->C, task->r);
	}

	// A total count is worth summing now rather than later
	plan->tasks = plan->pCount + plan->aCount;
	
	fclose(fin);
	return plan;
}

void CleanPlan(SimPlan* plan) {
	// Cleanup pTasks
	for (int i = 0; i < plan->pCount; ++i) {
		free(plan->pTasks[i].ID);
	}
	free(plan->pTasks);
	
	// Cleanup aTasks
	for (int i = 0; i < plan->aCount; ++i) {
		free(plan->aTasks[i].ID);
	}
	free(plan->aTasks);
	
	// Cleanup plan
	free(plan);
}
