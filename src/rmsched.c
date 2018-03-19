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

void checkReleases(RunningTask* task_set, uint8_t pCount, int msec, Schedule* sched) {
	for (int i = 0; i < pCount; i++) {
		task_set[i].P = task_set[i].periodicTask->T - ((msec+1)%task_set[i].periodicTask->T);
		if (task_set[i].P == task_set[i].periodicTask->T) {
			if (task_set[i].R != task_set[i].periodicTask->C) {
//				reportDeadlineMissed(plan, task_set[i]->columnIndex);
				sched->flags[(msec * sched->tasks) + task_set[i].periodicTask->taskIndex] = STATUS_OVERDUE;


				task_set[i].R = task_set[i].periodicTask->C;
			}
			task_set[i].ran = false;
		}
	}
}

uint8_t checkToRun(RunningTask* periodicTasks, uint8_t pCount) {
	for (int i = 0; i < pCount; i++) {
		if (!periodicTasks[i].ran) {
			return i;
		}
	}
	return pCount;
}

Schedule* RmSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	//
	RunningTask* task_set = (RunningTask*)calloc(sizeof(RunningTask), plan->pCount);
//	RunningAperiodic* task_set = (RunningAperiodic*)calloc(sizeof(RunningAperiodic), plan->aCount);
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
//        for (int deadline = task_set[i].periodicTask->T; deadline < plan->duration;
//			 deadline += task_set[i].periodicTask->T) {
            //find deadline, iterate through
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
                if (release == time) {
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
//        for (int i = 0; i < plan->duration; i++) {
//            running = checkToRun(task_set, plan->pCount);
//            if (running < plan->pCount) {
//
//
//                task_set[running].R--;
//                if (task_set[running].R == 0) {
//                    task_set[running].ran = true;
//                    task_set[running].R = task_set[running].periodicTask->C;
//                }
//                if (previous != plan->pCount &&
//                    task_set[running].periodicTask->ID != task_set[previous].periodicTask->ID &&
//                    task_set[previous].R != task_set[previous].periodicTask->C) {
//                    sched->flags[((i - 1) * plan->tasks) + task_set[previous].periodicTask->columnIndex -
//                                 1] = STATUS_PREEMPTED;
//                }
//            }
//
//            checkReleases(task_set, plan->pCount, i, sched);
//            previous = running;
//        }
    }
	free(task_set);
	//
	return sched;
}
