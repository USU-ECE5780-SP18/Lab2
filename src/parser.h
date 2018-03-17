#pragma once
#include <stdint.h>

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t T;
	uint8_t rIndex;
} PeriodicTask;

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t r;
	uint8_t rIndex;
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
