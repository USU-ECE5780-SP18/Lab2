#include "parser.h"
#include "reporter.h"
#include "sched.h"
#include <stdlib.h>

void sortTasks(RunningPeriodic* tasks, uint8_t pCount) {
	//put tasks in order by priority
	for (int i = 0; i < pCount; i++) {
		for (int j = i+1; j < pCount; j++) {
			if (tasks[j].periodicTask->T < tasks[i].periodicTask->T) {
				RunningPeriodic temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j].periodicTask->T == tasks[i].periodicTask->T) {
				if (tasks[j].periodicTask->C > tasks[i].periodicTask->C) {
					RunningPeriodic temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

void checkReleases(RunningPeriodic* periodicTasks, uint8_t pCount, int msec, Schedule* sched) {
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

uint8_t checkToRun(RunningPeriodic* periodicTasks, uint8_t pCount) {
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
	RunningPeriodic* task_set = (RunningPeriodic*)calloc(sizeof(RunningPeriodic), plan->pCount);
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

        uint8_t running;
        uint8_t previous = 0;

        int deadline;
        for (int j = 0; j < plan->duration; j++) {
            //find deadline, iterate through
//            periodicTasks[i].P = periodicTasks[i].periodicTask->T - ((msec+1)%periodicTasks[i].periodicTask->T);
            deadline = task_set[i].periodicTask->T + j;
            if (deadline > plan->duration) deadline = plan->duration;
            printf("here %d\n", deadline);
            j = task_set[i].R;
            for (int k = deadline - 1; j > 0; k--){
                if (sched->activeTask[k] == 0){
                    sched->activeTask[k] = task_set[i].periodicTask->rIndex;
                    j--;
                } else {
//                    k++;
                }
                if (previous == k) {
                    sched->flags[(k * sched->tasks) + periodicTasks[i].periodicTask->rIndex - 1] = STATUS_OVERDUE;
                    break;
                }
            }
            j = deadline - 1;
            previous = j;

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
//                    sched->flags[((i - 1) * plan->tasks) + task_set[previous].periodicTask->rIndex -
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
