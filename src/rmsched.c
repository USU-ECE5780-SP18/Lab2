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

	bool preemptFlag = false;

	uint8_t task; // index of pTask or aTask marking the active task

	uint16_t
		time, // marker for the current time while iterating
		runtime, // the amount of time left to schedule for the current task
		release, // the time at which the current task was released
		deadline; // the time at which the current task will have missed its deadline

	// Generate a list of periodic tasks sorted by shortest period first
	PeriodicTask** pTasks = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++) {
		pTasks[i] = plan->pTasks + i;
	}
	sortTasks(pTasks, plan->pCount);

	// Generate the schedule ALAP in order of the highest priority periodic tasks
	for (task = 0; task < plan->pCount; task++) {
		bool incompletePeriod = false;
		deadline = release = 0;

		// Schedule all periods for the given task with whatever space is left in the schdule
		// (Current task is the higest priority among unscheduled tasks)
		while (deadline < plan->duration) {
			uint16_t finalPreempt = 0;
			runtime = pTasks[task]->C;
			preemptFlag = false;

			// Increment at the start of the loop to catch an incomplete period
			deadline += pTasks[task]->T;
			if (deadline > plan->duration) {
				deadline = plan->duration;
				incompletePeriod = true;
			}

			// Iterate backwards in time so it is scheduled as late as possible
			for (time = deadline - 1; runtime > 0; time--) {
				if (sched->activeTask[time] == 0) {
					// If not yet scheduled, it is the time at which we are preempted before missing a deadline
					if (finalPreempt == 0) {
						finalPreempt = time;
					}

					// If the next task in the schedule is different we are about to be preempted
					if (preemptFlag) {
						sched->flags[((time)* sched->tasks) + pTasks[task]->taskIndex] = STATUS_PREEMPTED;
					}

					// Signal to the next loop (time - 1) that at this point (time) the current task was running
					preemptFlag = false;

					// Schedule the current job for the given cycle
					sched->activeTask[time] = pTasks[task]->columnIndex;
					runtime--;
				}

				// If we have executed but not not at the current time signal preemption to the next loop (time - 1)
				else if (runtime != pTasks[task]->C) {
					preemptFlag = true;
				}

				// If we've iterated back to the release time then this task was unable to meet it's deadline
				if (release == time && runtime != 0) {
					// Or the deadline is past the simulation's end point and we don't know
					if (!incompletePeriod) {
						// The last time this task is scheduled for is the time when it's preempted
						sched->flags[(finalPreempt * sched->tasks) + pTasks[task]->taskIndex] = STATUS_PREEMPTED;

						// May overwrite the previous status if we were able to schedule at the deadline and that's ok
						sched->flags[((deadline - 1) * sched->tasks) + pTasks[task]->taskIndex] = STATUS_OVERDUE;
					}
					break;
				}
			}

			// The release time of the (n+1)'th period of the given task is the deadline of the n'th period
			release = deadline;
		}
	}
	free(pTasks);

	// Generate a list of aperiodic tasks sorted by earliest release time first
	AperiodicTask** aTasks = (AperiodicTask**)calloc(sizeof(AperiodicTask*), plan->aCount);
	for (int i = 0; i < plan->aCount; i++) {
		aTasks[i] = plan->aTasks + i;
	}
	sortTasks((PeriodicTask**)aTasks, plan->aCount);

	task = 0;
	time = aTasks[task]->r;
	runtime = aTasks[task]->C;
	preemptFlag = false;
	deadline = time + 500;

	while (time < plan->duration && task < plan->aCount) {
		if (sched->activeTask[time] == 0) {
			preemptFlag = true;
			sched->activeTask[time] = aTasks[task]->columnIndex;
			runtime--;
			if (runtime == 0) {
				task++;
				preemptFlag = false;
				runtime = aTasks[task]->C;
				deadline = aTasks[task]->r + 500;
				if (aTasks[task]->r > time) {
					time = aTasks[task]->r;
					continue;
				}
			}
		}
		else if (preemptFlag) {
			sched->flags[((time - 1) * sched->tasks) + aTasks[task]->taskIndex] = STATUS_PREEMPTED;
			preemptFlag = false;
		}
		time++;
		while (deadline == time) {
			sched->flags[((time - 1) * sched->tasks) + aTasks[task]->taskIndex] = STATUS_OVERDUE;
			if (++task >= plan->aCount) { break; }
			preemptFlag = false;
			runtime = aTasks[task]->C;
			deadline = aTasks[task]->r + 500;
			if (aTasks[task]->r > time) {
				time = aTasks[task]->r;
			}
		}
	}
	free(aTasks);

	return sched;
}
