#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------------------------------------------------+
// Helper which decreases code duplication in the parsing of periodic and aperiodic tasks from the input file          |
// Exploits the symmetry of struct PeriodicTask and struct AperiodicTask                                               |
// Exploits the symmetry of the input file format "ID, C, T/r" for Periodic/Aperiodic                                  |
//---------------------------------------------------------------------------------------------------------------------+
static inline void ParseTask(char* buff, size_t line_n, PeriodicTask* task) {
	// Indicies of string parsing bounds
	size_t
		bos = 0, // beginning of string
		eos = 0; // end of string

	// Get the ID
	{
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here

		task->ID = (char*)malloc(eos + 1);
		memcpy(task->ID, buff, eos);
		task->ID[eos] = 0;
	}

	// Get the execution time
	{
		bos = ++eos; // buff + eos => ", ", buff + (++eos) => " "
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here

		buff[eos] = 0; // replace comma with a null pointer to aid the atoi function
		task->C = atoi(buff + bos);
	}

	// Get the period (for periodic) or the absolute release time (for aperiodic)
	{
		bos = ++eos; // buff + eos => "\0 ", buff + (++eos) => " "
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here

		buff[eos] = 0; // replace comma with a null pointer to aid the atoi function
		task->T = atoi(buff + bos); // same memory location and size for (AperiodicTask*)->r
	}
}

//---------------------------------------------------------------------------------------------------------------------+
// Returns a SimPlan struct with the relevant task settings based on data parsed from the given input file             |
//---------------------------------------------------------------------------------------------------------------------+
SimPlan* ParsePlan(const char* file) {
	SimPlan* plan = (SimPlan*)calloc(sizeof(SimPlan), 1);
	FILE* fin = fopen(file, "r");

	size_t buffsize = 128;
	char* buff = (char*)calloc(sizeof(char), buffsize);

	size_t line_n; // the length of each line (including 2 for \r\n)

	{
		// Parse the file to get pCount
		fgets(buff, buffsize, fin);
		plan->pCount = atoi(buff);

		// Parse the file to get time
		fgets(buff, buffsize, fin);
		plan->duration = atoi(buff);

		if (plan->pCount > 0) {
			plan->pTasks = (PeriodicTask*)calloc(sizeof(PeriodicTask), plan->pCount);
		}
	}
	printf("Time: %i\npCount: %i\n", plan->duration, plan->pCount);

	// Parse the file pCount times to get the data for each periodic task
	for (uint8_t pTask = 0; pTask < plan->pCount; ++pTask) {
		line_n = getline(&buff, &buffsize, fin);
		PeriodicTask* task = (plan->pTasks) + pTask;
		ParseTask(buff, line_n, task);
		task->taskIndex = pTask;
		task->columnIndex = pTask + 1;
		printf("pTasks[%i]: {ID: \"%s\", C: %i, T: %i}\n", pTask, task->ID, task->C, task->T);
	}

	// Parse the file to get aCount (optional parameter)
	char* optional = fgets(buff, buffsize, fin);
	if (optional != NULL) {
		plan->aCount = atoi(buff);
		if (plan->aCount > 0) {
			plan->aTasks = (AperiodicTask*)calloc(sizeof(AperiodicTask), plan->aCount);
		}
	}
	printf("aCount: %i\n", plan->aCount);

	// Parse the file aCount times to get the data for each periodic pTask
	for (uint8_t aTask = 0; aTask < plan->aCount; ++aTask) {
		line_n = getline(&buff, &buffsize, fin);
		AperiodicTask* task = (plan->aTasks) + aTask;
		ParseTask(buff, line_n, (PeriodicTask*)task);
		task->taskIndex = plan->pCount + aTask;
		task->columnIndex = plan->pCount + aTask + 1;
		printf("aTasks[%i]: {ID: \"%s\", C: %i, r: %i}\n", aTask, task->ID, task->C, task->r);
	}

	// A total count is worth summing now rather than later
	plan->tasks = plan->pCount + plan->aCount;

	fclose(fin);
	return plan;
}

void CleanPlan(SimPlan* plan) {
	// Cleanup pTasks
	for (uint8_t pTask = 0; pTask < plan->pCount; ++pTask) {
		free(plan->pTasks[pTask].ID);
	}
	free(plan->pTasks);

	// Cleanup aTasks
	for (uint8_t aTask = 0; aTask < plan->aCount; ++aTask) {
		free(plan->aTasks[aTask].ID);
	}
	free(plan->aTasks);

	// Cleanup plan
	free(plan);
}
