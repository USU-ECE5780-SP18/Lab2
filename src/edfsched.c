#include "parser.h"
#include "reporter.h"
#include "sched.h"
#include <stdlib.h>

void computePriority(RunningPeriodic* tasks, uint8_t pCount) {
	for (int i = 0; i < pCount; i++) {
		for (int j = i+1; j < pCount; j++) {
			if (tasks[j].P < tasks[i].P && !tasks[i].ran) {
				RunningPeriodic temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j].P == tasks[i].P) {
				if (tasks[j].R < tasks[i].R) {
					RunningPeriodic temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

Schedule* EdfSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	RunningPeriodic* task_set = (RunningPeriodic*)calloc(sizeof(RunningPeriodic), plan->pCount);
	for (int i = 0; i < plan->pCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].periodicTask = plan->pTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].periodicTask->C;
	}

	sortTasks(task_set, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->duration; i++) {
		running = checkToRun(task_set, plan->pCount);
		if (running < plan->pCount) {

			sched->activeTask[i] = task_set[running].periodicTask->rIndex;

			task_set[running].R--;
			if (task_set[running].R == 0) {
				task_set[running].ran = true;
				task_set[running].R = task_set[running].periodicTask->C;
			}
			if (previous != plan->pCount &&
				task_set[running].periodicTask->ID != task_set[previous].periodicTask->ID &&
				task_set[previous].R != task_set[previous].periodicTask->C) {
				sched->flags[((i-1) * plan->tasks) + task_set[previous].periodicTask->rIndex - 1] = STATUS_PREEMPTED;
			}

		}
		previous = running;
		checkReleases(task_set, plan->pCount, i, sched);
		computePriority(task_set, plan->pCount);
	}
	free(task_set);
	return sched;
}
