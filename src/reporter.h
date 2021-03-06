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

	// flattened array of dimensions [duration][tasks]
	char* flags;

	// sum of response times of each aperiodic task (division by aCount done by the reporter)
	uint16_t aperiodicResponseTimes;
	uint8_t aCount;
} Schedule;

void WriteSchedule(FILE* fout, Schedule* schedule);
Schedule* MakeSchedule(SimPlan* plan);
void CleanSchedule(Schedule* schedule);
