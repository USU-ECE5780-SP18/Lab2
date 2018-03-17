#pragma once
#include <stdint.h>

typedef struct {
    PeriodicTask* periodicTask;

    uint16_t R;
    bool ran;
    uint16_t P;
} RunningPeriodic;

typedef struct {
    AperiodicTask* aperiodicTask;
//
//	uint16_t R;
//	bool ran;
//	uint16_t P;
} RunningAperiodic;

extern void checkReleases(RunningPeriodic* periodicTasks, uint8_t pCount, int msec, Schedule* sched);
extern void sortTasks(RunningPeriodic* tasks, uint8_t pCount);
extern uint8_t checkToRun(RunningPeriodic* periodicTasks, uint8_t pCount);
