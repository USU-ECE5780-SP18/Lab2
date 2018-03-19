#include "parser.h"
#include "reporter.h"
#include "sched.h"
#include <stdlib.h>

void sortTasks(RunningTask* tasks, uint8_t pCount) {
	//put tasks in order by priority
	for (int i = 0; i < pCount; i++) {
		for (int j = i+1; j < pCount; j++) {
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

Schedule* RmSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);
	RunningTask* task_set = (RunningTask*)calloc(sizeof(RunningTask), plan->pCount);
    for (int i = 0; i < plan->pCount; i++) {
        //bool to help prioritize
        task_set[i].ran = false;
        task_set[i].periodicTask = plan->pTasks + i;
        //the task runtime
        task_set[i].R = task_set[i].periodicTask->C;
    }
    sortTasks(task_set, plan->pCount);
    for (int i = 0; i < plan->pCount; i++) {

        uint16_t runtime;
        uint16_t release = 0;

		bool hardDeadline = true;
        int deadline = 0;
		while(deadline < plan->duration){
			deadline += task_set[i].periodicTask->T;
			if (deadline > plan->duration){
				deadline = plan->duration;
				hardDeadline = false;
			}
			runtime = task_set[i].periodicTask->C;
			bool preempted = false;
			int fake_preemption = 0;
            for (int time = deadline - 1; runtime > 0; time--){
                if (sched->activeTask[time] == 0) {
					if (fake_preemption == 0){
						fake_preemption = time;
					}
					if (runtime != task_set[i].periodicTask->C && preempted){
						sched->flags[((time) * sched->tasks) + task_set[i].periodicTask->taskIndex] = STATUS_PREEMPTED;
					}
					preempted = false;
					sched->activeTask[time] = task_set[i].periodicTask->columnIndex;
					runtime--;
				} else {
					preempted = true;
				}
                if (release == time && runtime != 0) {
					if (hardDeadline){
						sched->flags[((fake_preemption) * sched->tasks) + task_set[i].periodicTask->taskIndex] = STATUS_PREEMPTED;
						sched->flags[((deadline-1) * sched->tasks) + task_set[i].periodicTask->taskIndex] = STATUS_OVERDUE;
					}
					printf("missed a deadline at %d\n", time);
                    break;
                }
            }
            release = deadline;
			printf("release %d\n", release);
        }
    }

	free(task_set);
	task_set = (RunningTask*)calloc(sizeof(RunningTask), plan->aCount);
	for (int i = 0; i < plan->aCount; i++) {
		//bool to help prioritize
		task_set[i].ran = false;
		task_set[i].aperiodicTask = plan->aTasks + i;
		//the task runtime
		task_set[i].R = task_set[i].aperiodicTask->C;
		//to reuse sortTasks
		task_set[i].periodicTask = (PeriodicTask*)task_set[i].aperiodicTask;
	}

	sortTasks(task_set, plan->aCount);
	int time = task_set[0].aperiodicTask->r;
	int aTasks = 0;
	int running = task_set[0].aperiodicTask->C;
	bool preemption = false;
	int deadline = time + 500;

	while (time < plan->duration && aTasks < plan->aCount){
		if (sched->activeTask[time] == 0) {
			preemption = true;
			sched->activeTask[time] = task_set[aTasks].periodicTask->columnIndex;
			running--;
			if (running == 0) {
				aTasks++;
				preemption = false;
				running = task_set[aTasks].aperiodicTask->C;
				deadline = task_set[aTasks].aperiodicTask->r + 500;
				if (task_set[aTasks].aperiodicTask->r > time) {
					time = task_set[aTasks].aperiodicTask->r;
					continue;
				}
			}
		} else if (preemption){
			sched->flags[((time-1) * sched->tasks) + task_set[aTasks].periodicTask->taskIndex] = STATUS_PREEMPTED;
			preemption = false;
		}
		time++;
		while (deadline == time){
			sched->flags[((time-1) * sched->tasks) + task_set[aTasks].periodicTask->taskIndex] = STATUS_OVERDUE;
			aTasks++;
			preemption = false;
			running = task_set[aTasks].aperiodicTask->C;
			deadline = task_set[aTasks].aperiodicTask->r + 500;
			if (task_set[aTasks].aperiodicTask->r > time) {
				time = task_set[aTasks].aperiodicTask->r;
			}
		}

//		if (completed) aTasks++;
	}

	free(task_set);
	return sched;
}
