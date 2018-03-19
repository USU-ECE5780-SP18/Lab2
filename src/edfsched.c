#include "parser.h"
#include "reporter.h"
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
	ListNode* next;
	ListNode* prev;
} ListNode;

void CleanNode(ListNode* node) {
	free(node->value);
	free(node);
}

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

	// Fill the release schedule with all aperiodic tasks (implicit deadline of 500 milliseconds)
	for (int aTask = 0; aTask < plan->aCount; ++aTask) {
		AperiodicTask* task = (plan->aTasks) + aTask;
		
		// Create the job
		Job* job = (Job*)malloc(sizeof(Job));
		job->genericTask = (PeriodicTask*)task;
		job->periodicTask = NULL;
		job->aperiodicTask = task;
		job->R = task->C;
		job->d = (task->r + 500);

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

	for (int time = 0; time < sched->duration; ++time) {
		char* flagsPrev = sched->flags + ((time - 1) * sched->tasks);
		char* flagsNow = sched->flags + (time * sched->tasks);

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

				// Add active to the wait list
				if (active != NULL) {
					active->next = wait;
					if (wait != NULL) {
						wait->prev = active;
					}

					// Do a final check to make sure we didn't just switch to active (i.e. it's not actually being preempted)
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

		if (active != NULL) {
			sched->activeTask[time] = active->value->genericTask->columnIndex;
			active->value->R--;

			bool closeJob = false;

			// finished
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
					if (active != NULL) {
						if (active->value->d == time + 1) {
							flagsNow[active->value->genericTask->taskIndex] = STATUS_OVERDUE;
							CleanNode(active);
							active = NULL;
						}
						else {
							break;
						}
					}
					else {
						break;
					}
				}
			}
		}
	}

	// By the end releaseSchedule is empty
	free(releaseSchedule);

	return sched;
}
