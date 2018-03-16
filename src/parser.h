#pragma once
#include <stdint.h>

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t T;
} PeriodicTask;

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t r;
} AperiodicTask;

typedef struct {
	uint16_t duration;
	
	uint8_t pCount;
	PeriodicTask* pTasks;
	
	uint8_t aCount;
	AperiodicTask* aTasks;
} SimPlan;

SimPlan* ParsePlan(const char* file);
void CleanPlan(SimPlan* plan);
