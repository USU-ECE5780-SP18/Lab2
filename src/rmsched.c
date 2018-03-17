#include "parser.h"
#include "reporter.h"
#include "sched.h"
#include <stdlib.h>

void sortRM(RunningTask* tasks, uint8_t total) {
	//put tasks in order by priority
	for (int i = 0; i < total; i++) {
		for (int j = i+1; j < total; j++) {
			if (tasks[j].periodicTask->T < tasks[i].periodicTask->T) {
				RunningTask temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j].periodicTask->T == tasks[i].periodicTask->T) {
				if (tasks[j].periodicTask->C > tasks[i].periodicTask->C) {
					RunningTask temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

void checkReleases(RunningTask* periodicTasks, uint8_t pCount, int msec, Schedule* sched) {
	for (int i = 0; i < pCount; i++) {
		periodicTasks[i].P = periodicTasks[i].periodicTask->T - ((msec+1)%periodicTasks[i].periodicTask->T);
		if (periodicTasks[i].P == periodicTasks[i].periodicTask->T) {
			if (periodicTasks[i].R != periodicTasks[i].periodicTask->C) {
//				reportDeadlineMissed(plan, periodicTasks[i]->rIndex);
				sched->flags[(msec * sched->tasks) + periodicTasks[i].periodicTask->rIndex - 1] = STATUS_OVERDUE;


				periodicTasks[i].R = periodicTasks[i].periodicTask->C;
			}
			periodicTasks[i].ran = false;
		}
	}
}

uint8_t checkToRun(RunningTask* tasks, uint8_t total) {
	for (int i = 0; i < total; i++) {
		if (!tasks[i].ran) {
			return i;
		}
	}
	return total;
}

Schedule* RmSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	//
	RunningTask* task_set = (RunningTask*)calloc(sizeof(RunningTask), plan->tasks);
	for (int i = 0; i < plan->pCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].periodicTask = plan->pTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].periodicTask->C;
		task_set[i].isPeriodic = true;
	}
	for (int i = 0; i < plan->aCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].aperiodicTask = plan->aTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].aperiodicTask->C;
		task_set[i].isPeriodic = false;
	}

	//TODO change past here
	sortRM(task_set, plan->pCount);
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

		checkReleases(task_set, plan->pCount, i, sched);
		previous = running;
	}
	free(task_set);
	//
	return sched;
}
