#pragma once
#include <stdint.h>

// Per the assignment description, aperiodic tasks have an implicit deadline of 500ms from the release time
#define APERIODIC_DEADLINE 500

typedef struct {
	uint8_t taskIndex;
	uint8_t columnIndex;
	char* ID;
	uint16_t C;
	uint16_t T;
} PeriodicTask;

typedef struct {
	uint8_t taskIndex;
	uint8_t columnIndex;
	char* ID;
	uint16_t C;
	uint16_t r;
} AperiodicTask;

typedef struct {
	uint16_t duration;
	uint8_t tasks;
	
	uint8_t pCount;
	PeriodicTask* pTasks;
	
	uint8_t aCount;
	AperiodicTask* aTasks;
} SimPlan;

SimPlan* ParsePlan(const char* file);
void CleanPlan(SimPlan* plan);
