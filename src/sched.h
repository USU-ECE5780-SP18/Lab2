#pragma once
#include <stdint.h>

typedef struct {
    PeriodicTask* periodicTask;
    AperiodicTask* aperiodicTask;

    uint16_t R;
    bool ran;
    bool isPeriodic;
    uint16_t P;
} RunningTask;

extern void checkReleases(RunningTask* periodicTasks, uint8_t pCount, int msec, Schedule* sched);
extern uint8_t checkToRun(RunningTask* tasks, uint8_t total);
