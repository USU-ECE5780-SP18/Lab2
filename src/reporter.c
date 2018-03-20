#include "parser.h"
#include "reporter.h"
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------------------------------------------------+
// Outputs a number to the given column in the table (0 - 9999)                                                        |
//---------------------------------------------------------------------------------------------------------------------+
static inline void colCounter(char* col, uint16_t n) {
	// Two spaces before the number
	col[0] = col[1] = ' ';

	// The number (4 digit limit)
	n = n % 10000;
	col[2] = (n / 1000) + '0'; n %= 1000;
	col[3] = (n / 100) + '0'; n %= 100;
	col[4] = (n / 10) + '0'; n %= 10;
	col[5] = n + '0';

	// Two spaces after the number
	col[6] = col[7] = ' ';
}

//---------------------------------------------------------------------------------------------------------------------+
// Generates the output table in the given file for a fully generated schedule                                         |
//---------------------------------------------------------------------------------------------------------------------+
void WriteSchedule(FILE* fout, Schedule* sched) {
	uint8_t* activeTask = sched->activeTask;
	char* flags = sched->flags;

	//                     9 => 8-char wide column plus one edge of the column border
	//                                       + 1 for the time column
	//                                             + 1 for the table edge
	uint16_t tableWidth = (9 * (sched->tasks + 1)) + 1;

	//                                    + 3 for \r\n\0
	char* buff = (char*)malloc(tableWidth + 3);
	memcpy(buff + tableWidth, "\r\n\0", 3);

	// Counter variable for utilization
	uint16_t utilization = 0;

	// Counter variables for preemption and missed deadlines
	uint8_t* pCount = (uint8_t*)calloc(sizeof(uint8_t), sched->tasks);
	uint8_t* dCount = (uint8_t*)calloc(sizeof(uint8_t), sched->tasks);

	// Generate the first row of the table: "+---+"
	{
		buff[0] = buff[tableWidth - 1] = '+';
		for (uint16_t i = 1; i < tableWidth - 1; ++i) {
			buff[i] = '-';
		}
		fprintf(fout, "%s", buff);
	}

	// Generate the column header: "|  Time  | TaskId |  ....  |"
	{
		memcpy(buff, "|  Time  | ", 10);
		char* current = buff + 10;
		for (uint8_t task = 0; task < sched->tasks; ++task) {
			// Fills each column header with a given id text truncated to 6 digits and centered
			// Includes spaces on either side and a right side column separator
			char* id = sched->header[task];
			size_t len = strlen(id);

			// Left side space
			*current = ' ';
			++current;

			if (len >= 6) {
				// The (potentially) truncated id
				memcpy(current, id, 6);
			}
			else {
				// spaces prepended to the id
				size_t lhs = (6 - len) / 2;
				for (size_t j = 0; j < lhs; ++j) {
					current[j] = ' ';
				}

				// The centered id
				memcpy(current + lhs, id, len);

				// spaces appended to the id
				for (size_t j = lhs + len; j < 6; ++j) {
					current[j] = ' ';
				}
			}
			current = current + 6;

			// Right side space and column edge
			memcpy(current, " |", 2);
			current = current + 2;
		}
		fprintf(fout, "%s", buff);
	}

	// Generate separator row: "|---|---|---|"
	{
		for (uint16_t i = 0; i < tableWidth; ++i) {
			buff[i] = i % 9 == 0 ? '|' : '-';
		}
		fprintf(fout, "%s", buff);
	}

	// Output data
	for (uint16_t now = 0; now < sched->duration; ++now) {
		// Print the time
		colCounter(buff + 1, now);

		// Clear each column
		for (uint8_t task = 0; task < sched->tasks; ++task) {
			//          + 1 => Skip the left table edge
			//                 9 * => each column is 8 digits wide plus a column edge
			//                           + 1 => skip the time column
			memcpy(buff + 1 + (9 * (task + 1)), "        ", 8);
		}

		// Star the actively running task
		uint8_t active = activeTask[now];
		if (active != 0 && active <= sched->tasks) {
			++utilization;

			// Compiler should simplify the arithmetic to two operations, left more for clarity
			buff[1 + (9 * (active)) + 4] = '*';
		}

		// Apply other flags to all relevant tasks
		for (uint8_t task = 0; task < sched->tasks; ++task) {
			char flag = flags[(now * sched->tasks) + task];

			switch (flag) {
				case STATUS_OVERDUE:
					++(dCount[task]);
					break;
				case STATUS_PREEMPTED:
					++(pCount[task]);
					break;
			}

			// Compiler should simplify the arithmetic to two operations, left more for clarity
			buff[1 + (9 * (task + 1)) + 3] = flag;
		}

		fprintf(fout, "%s", buff);
	}

	uint8_t dTotal = 0;
	uint8_t pTotal = 0;

	// Generate separator row: "|---|---|---|"
	{
		for (uint16_t i = 0; i < tableWidth; ++i) {
			buff[i] = i % 9 == 0 ? '|' : '-';
		}
		fprintf(fout, "%s", buff);
	}

	// Generate dCount stats row
	{
		memcpy(buff + 1, " dCount ", 8);

		for (uint8_t task = 0; task < sched->tasks; ++task) {
			uint8_t cnt = dCount[task];
			dTotal += cnt;
			colCounter(buff + 1 + (9 * (task + 1)), cnt);
		}
		fprintf(fout, "%s", buff);
	}

	// Generate pCount stats row
	{
		memcpy(buff + 1, " pCount ", 8);

		for (uint8_t task = 0; task < sched->tasks; ++task) {
			uint8_t cnt = pCount[task];
			pTotal += cnt;
			colCounter(buff + 1 + (9 * (task + 1)), cnt);
		}

		fprintf(fout, "%s", buff);
	}

	// Generate the last row of the table: "+---+"
	{
		buff[0] = buff[tableWidth - 1] = '+';
		for (uint16_t i = 1; i < tableWidth - 1; ++i) {
			buff[i] = '-';
		}
		fprintf(fout, "%s", buff);
	}

	// Print the summary statistics
	fprintf(fout,
		"Utilization: %.4f\r\nMissed Deadlines: %i\r\nPreemption Count: %i\r\n",
		((float)utilization) / sched->duration, dTotal, pTotal);

	free(dCount);
	free(pCount);
	free(buff);
}

//---------------------------------------------------------------------------------------------------------------------+
// Dynamically allocates (must call CleanSchedule) space for a schedule with the right dimensions and a clean state    |
// Also pre-fills the schedule with the released flag since it's convienient and independent of the type of schedule   |
//---------------------------------------------------------------------------------------------------------------------+
Schedule* MakeSchedule(SimPlan* plan) {
	Schedule* sched = (Schedule*)malloc(sizeof(Schedule));
	sched->duration = plan->duration;
	sched->tasks = plan->tasks;

	// Create active task table (calloc initializes to 0 => slack)
	sched->activeTask = (uint8_t*)calloc(sizeof(uint8_t), sched->duration);

	// Create average response-time table (calloc initializes to 0 for convenience when dealing with sums)
	sched->responseTimes = (uint16_t*)calloc(sizeof(uint16_t), sched->tasks);
	sched->responseCount = (uint8_t*)calloc(sizeof(uint8_t), sched->tasks);

	// Auto-fill the headers based on the task ID's in the given plan
	sched->header = (char**)malloc(sizeof(char*) * sched->tasks);
	for (uint8_t pTask = 0; pTask < plan->pCount; ++pTask) {
		PeriodicTask* task = plan->pTasks + pTask;
		sched->header[task->taskIndex] = task->ID;
	}
	for (uint8_t aTask = 0; aTask < plan->aCount; ++aTask) {
		AperiodicTask* task = plan->aTasks + aTask;
		sched->header[task->taskIndex] = task->ID;
	}

	// Clear status state for all tasks at all times
	uint32_t flag_n = sched->duration * sched->tasks;
	sched->flags = (char*)malloc(sizeof(char) * flag_n);
	for (uint32_t flag = 0; flag < flag_n; ++flag) {
		sched->flags[flag] = STATUS_NONE;
	}

	// Release times are independent of schedule, so generate them up-front
	for (uint8_t pTask = 0; pTask < plan->pCount; ++pTask) {
		PeriodicTask* task = plan->pTasks + pTask;
		for (uint16_t release = 0; release < sched->duration; release += task->T) {
			sched->flags[(release * plan->tasks) + task->taskIndex] = STATUS_RELEASED;
		}
	}
	for (uint8_t aTask = 0; aTask < plan->aCount; ++aTask) {
		AperiodicTask* task = plan->aTasks + aTask;
		sched->flags[(task->r * plan->tasks) + task->taskIndex] = STATUS_RELEASED;
	}

	return sched;
}

//---------------------------------------------------------------------------------------------------------------------+
// Appropriately frees the memory associated with the given schedule                                                   |
//---------------------------------------------------------------------------------------------------------------------+
void CleanSchedule(Schedule* schedule) {
	free(schedule->activeTask);
	free(schedule->header);
	free(schedule->flags);
	free(schedule->responseTimes);
	free(schedule->responseCount);
	free(schedule);
}
