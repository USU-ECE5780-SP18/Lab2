#pragma once
#include <stdint.h>
#include <stdio.h>

enum
{
	STATUS_NONE         = ' ',
	STATUS_RELEASED     = 'r',
	STATUS_PREEMPTED    = 'p',
	STATUS_OVERDUE      = 'd',
};

typedef struct {
	uint16_t duration;
	uint8_t tasks;
	char** header;
	
	// array of length `duration` values in [0, tasks] (0 => slack)
	uint8_t* activeTask;

	// array of length tasks indicating the average response time of each task
	uint16_t* avgResponse;
	
	// flattened array of dimensions [duration][tasks]
	char* flags;
} Schedule;

void WriteSchedule(FILE* fout, Schedule* schedule);
Schedule* MakeSchedule(SimPlan* plan);
void CleanSchedule(Schedule* schedule);
