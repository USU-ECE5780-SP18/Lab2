#include "parser.h"
#include "reporter.h"
#include "sched.h"
#include <stdlib.h>

void sortEDF(RunningTask* tasks, uint8_t total) {
	for (int i = 0; i < total; i++) {
		for (int j = i+1; j < total; j++) {
			if (tasks[j].P < tasks[i].P && !tasks[i].ran) {
				RunningTask temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j].P == tasks[i].P) {
				if (tasks[j].R < tasks[i].R) {
					RunningTask temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

Schedule* EdfSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	RunningTask* task_set = (RunningTask*)calloc(sizeof(RunningTask), plan->tasks);

	for (int i = 0; i < plan->pCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].periodicTask = plan->pTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].periodicTask->C;
		task_set[i].isPeriodic = true;
		task_set[i].P = task_set[i].periodicTask->T;
	}
	for (int i = 0; i < plan->aCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].aperiodicTask = plan->aTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].aperiodicTask->C;
		task_set[i].isPeriodic = false;
		task_set[i].P = task_set[i].aperiodicTask->r + 500;
	}

	sortEDF(task_set, plan->tasks);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->duration; i++) {
		running = checkToRun(task_set, plan->tasks);
		if (running < plan->tasks) {
			RunningTask& task = task_set[running];

			PeriodicTask* taskGeneric;

			if (task.isPeriodic) {
				AperiodicTask* taskRef = task.periodicTask;
				taskGeneric = taskRef;
			}
			else {
				AperiodicTask* taskRef = task.aperiodicTask;
				taskGeneric = (PeriodicTask*)taskRef;
			}

			sched->activeTask[i] = taskGeneric->rIndex;

			task.R--;
			if (task.R == 0) {
				task.ran = true;
				if (task.isPeriodic){
					task.R = taskGeneric->C;
				}
			}
			if (previous != plan->tasks &&
					taskGeneric->ID != task_set[previous].periodicTask->ID &&
				task_set[previous].R != task_set[previous].periodicTask->C) {
				sched->flags[((i-1) * plan->tasks) + task_set[previous].periodicTask->rIndex - 1] = STATUS_PREEMPTED;
			}

		}
		previous = running;
		checkReleases(task_set, plan->pCount, i, sched);
		sortEDF(task_set, plan->tasks);
	}
	free(task_set);
	return sched;
}
