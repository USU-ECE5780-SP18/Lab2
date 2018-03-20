#include "parser.h"
#include "reporter.h"
#include <stdbool.h>
#include <stdlib.h>

//---------------------------------------------------------------------------------------------------------------------+
// Helper function - sorts a list of tasks such that higher proirity is lower indexed in the given list                |
// Simple bubble sort: primary sort min (PeriodicTask*)->T; secondary sort max (PeriodicTask*)->C                      |
// For periodic tasks in rate monotonic scheduling, the shorter period has the higher priority                         |
// Exploits symmetry of (AperiodicTask*)->r to achieve earliest release priority for aperiodic tasks                   |
//---------------------------------------------------------------------------------------------------------------------+
void sortTasks(PeriodicTask** tasks, uint8_t pCount) {
	for (int i = 0; i < pCount; i++) {
		for (int j = i + 1; j < pCount; j++) {
			if (tasks[j]->T < tasks[i]->T) {
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			}
			else if (tasks[j]->T == tasks[i]->T && tasks[j]->C > tasks[i]->C) {
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------+
// Generates a schedule for rate monotonic where periodic tasks are scheduled ALAP                                     |
//---------------------------------------------------------------------------------------------------------------------+
Schedule* RmSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	PeriodicTask** pTasks = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++) {
		pTasks[i] = plan->pTasks + i;
	}
	sortTasks(pTasks, plan->pCount);

	// Generate the schedule ALAP in order of the highest priority periodic tasks
	for (int i = 0; i < plan->pCount; i++) {
		uint16_t runtime;
		uint16_t release = 0;

		bool hardDeadline = true;
		int deadline = 0;
		while (deadline < plan->duration) {
			deadline += pTasks[i]->T;
			if (deadline > plan->duration) {
				deadline = plan->duration;
				hardDeadline = false;
			}
			runtime = pTasks[i]->C;
			bool preempted = false;
			int fake_preemption = 0;
			for (int time = deadline - 1; runtime > 0; time--) {
				if (sched->activeTask[time] == 0) {
					if (fake_preemption == 0) {
						fake_preemption = time;
					}
					if (runtime != pTasks[i]->C && preempted) {
						sched->flags[((time)* sched->tasks) + pTasks[i]->taskIndex] = STATUS_PREEMPTED;
					}
					preempted = false;
					sched->activeTask[time] = pTasks[i]->columnIndex;
					runtime--;
				}
				else {
					preempted = true;
				}
				if (release == time && runtime != 0) {
					if (hardDeadline) {
						sched->flags[((fake_preemption)* sched->tasks) + pTasks[i]->taskIndex] = STATUS_PREEMPTED;
						sched->flags[((deadline - 1) * sched->tasks) + pTasks[i]->taskIndex] = STATUS_OVERDUE;
					}
					break;
				}
			}
			release = deadline;
		}
	}
	free(pTasks);

	AperiodicTask** aTasks = (AperiodicTask**)calloc(sizeof(AperiodicTask*), plan->aCount);
	for (int i = 0; i < plan->aCount; i++) {
		aTasks[i] = plan->aTasks + i;
	}

	sortTasks((PeriodicTask**)aTasks, plan->aCount);
	int task = 0;
	int time = aTasks[task]->r;
	int running = aTasks[task]->C;
	bool preemption = false;
	int deadline = time + 500;

	while (time < plan->duration && task < plan->aCount) {
		if (sched->activeTask[time] == 0) {
			preemption = true;
			sched->activeTask[time] = aTasks[task]->columnIndex;
			running--;
			if (running == 0) {
				task++;
				preemption = false;
				running = aTasks[task]->C;
				deadline = aTasks[task]->r + 500;
				if (aTasks[task]->r > time) {
					time = aTasks[task]->r;
					continue;
				}
			}
		}
		else if (preemption) {
			sched->flags[((time - 1) * sched->tasks) + aTasks[task]->taskIndex] = STATUS_PREEMPTED;
			preemption = false;
		}
		time++;
		while (deadline == time) {
			sched->flags[((time - 1) * sched->tasks) + aTasks[task]->taskIndex] = STATUS_OVERDUE;
			if (++task >= plan->aCount) { break; }
			preemption = false;
			running = aTasks[task]->C;
			deadline = aTasks[task]->r + 500;
			if (aTasks[task]->r > time) {
				time = aTasks[task]->r;
			}
		}
	}

	free(aTasks);
	return sched;
}
