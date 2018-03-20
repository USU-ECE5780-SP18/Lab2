#include "parser.h"
#include "reporter.h"
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	PeriodicTask* genericTask;
	PeriodicTask* periodicTask;
	AperiodicTask* aperiodicTask;

	uint16_t R;
	uint16_t d;
} Job;

typedef struct ListNode {
	Job* value;
	struct ListNode* next;
	struct ListNode* prev;
} ListNode;

static inline void CleanNode(ListNode* node) {
	free(node->value);
	free(node);
}

//---------------------------------------------------------------------------------------------------------------------+
// Generates a basic earliest deadline first schedule                                                                  |
//---------------------------------------------------------------------------------------------------------------------+
Schedule* EdfSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	ListNode** releaseSchedule = (ListNode**)calloc(sizeof(ListNode*), sched->duration);

	// Fill the release schedule with all periodic tasks
	for (int pTask = 0; pTask < plan->pCount; ++pTask) {
		PeriodicTask* task = (plan->pTasks) + pTask;
		int time = 0;
		while (time < sched->duration) {
			// Create the job
			Job* job = (Job*)malloc(sizeof(Job));
			job->genericTask = task;
			job->periodicTask = task;
			job->aperiodicTask = NULL;
			job->R = task->C;

			// Insert into the release schedule
			ListNode* node = (ListNode*)malloc(sizeof(ListNode));
			node->value = job;
			node->prev = NULL;
			node->next = releaseSchedule[time];
			if (node->next != NULL) {
				node->next->prev = node;
			}
			releaseSchedule[time] = node;

			// Set the deadline last, since it will also update our iterator
			job->d = (time += task->T);
		}
	}

	// Fill the release schedule with all aperiodic tasks
	for (int aTask = 0; aTask < plan->aCount; ++aTask) {
		AperiodicTask* task = (plan->aTasks) + aTask;

		// Create the job
		Job* job = (Job*)malloc(sizeof(Job));
		job->genericTask = (PeriodicTask*)task;
		job->periodicTask = NULL;
		job->aperiodicTask = task;
		job->R = task->C;
		job->d = task->r + APERIODIC_DEADLINE;

		// Insert into the release schedule
		ListNode* node = (ListNode*)malloc(sizeof(ListNode));
		node->value = job;
		node->prev = NULL;
		node->next = releaseSchedule[task->r];
		if (node->next != NULL) {
			node->next->prev = node;
		}
		releaseSchedule[task->r] = node;
	}

	// Currently running task
	ListNode* active = NULL;

	// List of waiting tasks
	ListNode* wait = NULL;

	// There are two points of decision on which task executes at any given time:
	//   1 - when a task is released (preempt if one has an earlier deadline than the active task)
	//   2 - when a task completes (or stops due to missing its deadline) find the earliest deadline in wait
	for (int time = 0; time < sched->duration; ++time) {
		char* flagsPrev = sched->flags + ((time - 1) * sched->tasks);
		char* flagsNow = sched->flags + (time * sched->tasks);

		// First decision point: one or more tasks have been released
		if (releaseSchedule[time] != NULL) {
			ListNode* released = releaseSchedule[time];
			releaseSchedule[time] = NULL;

			ListNode* EarliestDeadline = active;
			ListNode* listIterator = released;
			if (active == NULL) {
				EarliestDeadline = listIterator;
				listIterator = listIterator->next;
			}

			// Find the earliest deadline among active and the released tasks
			// since active is earlier than anything in wait we can ignore wait
			while (listIterator != NULL) {
				if (listIterator->value->d < EarliestDeadline->value->d) {
					EarliestDeadline = listIterator;
				}
				listIterator = listIterator->next;
			}

			// There is a newly released job with an earlier deadline, preempt the active task
			if (EarliestDeadline != active) {
				ListNode* next = EarliestDeadline->next;
				ListNode* prev = EarliestDeadline->prev;

				// Remove Earliest from the list
				if (next != NULL) { next->prev = prev; }
				if (prev != NULL) { prev->next = next; }
				EarliestDeadline->next = EarliestDeadline->prev = NULL;

				// If the head is the earliest deadline shift the list which will later be merged into wait
				if (released == EarliestDeadline) {
					released = next;
				}

				// Preempt active and add it to the wait list
				if (active != NULL) {
					active->next = wait;
					if (wait != NULL) {
						wait->prev = active;
					}

					// Make sure we didn't just switch to active in a previous iteration of the loop (not preemption)
					if (sched->activeTask[time - 1] == active->value->genericTask->columnIndex) {
						flagsPrev[active->value->genericTask->taskIndex] = STATUS_PREEMPTED;
					}

					wait = active;
				}

				active = EarliestDeadline;
			}

			// Add released to wait
			if (released != NULL) {
				ListNode* tail = released;
				while (tail->next != NULL) {
					tail = tail->next;
				}

				tail->next = wait;
				if (wait != NULL) {
					wait->prev = tail;
				}
				wait = released;
			}
		}

		// Execute the active task - potentially deal with the second decision point: closeJob
		if (active != NULL) {
			sched->activeTask[time] = active->value->genericTask->columnIndex;
			active->value->R--;

			bool closeJob = false;

			// Job's finished (imagine an SCV's voice from starcraft)
			if (active->value->R == 0) {
				closeJob = true;
			}

			// Missed deadline
			else if (active->value->d == time + 1) {
				flagsNow[active->value->genericTask->taskIndex] = STATUS_OVERDUE;
				closeJob = true;
			}

			// if finished or will miss deadline pull next active from wait
			if (closeJob) {
				CleanNode(active);
				active = NULL;

				// Loop to make sure we handle multiple missed multiple deadlines as long as there are jobs in wait
				while (wait != NULL) {
					ListNode* EarliestDeadline = wait;
					ListNode* listIterator = wait->next;

					// Find the earliest deadline among waiting tasks
					while (listIterator != NULL) {
						if (listIterator->value->d < EarliestDeadline->value->d) {
							EarliestDeadline = listIterator;
						}
						listIterator = listIterator->next;
					}

					// remove EarliestDeadline from the list
					ListNode* next = EarliestDeadline->next;
					ListNode* prev = EarliestDeadline->prev;
					if (next != NULL) { next->prev = prev; }
					if (prev != NULL) { prev->next = next; }
					EarliestDeadline->next = EarliestDeadline->prev = NULL;

					// If the head is the earliest deadline shift the list
					if (wait == EarliestDeadline) {
						wait = next;
					}

					active = EarliestDeadline;

					// Check to make sure active is not going to miss its deadline as it's about to start
					if (active->value->d == time + 1) {
						flagsNow[active->value->genericTask->taskIndex] = STATUS_OVERDUE;
						CleanNode(active);
						active = NULL;
					}
					else {
						break;
					}
				}
			}
		}
	}

	// By the end releaseSchedule is empty because:
	// Each job has been transfered to wait, then freed one by one after entering the closeJob section
	free(releaseSchedule);

	return sched;
}
