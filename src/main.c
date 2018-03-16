#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	uint8_t rIndex;
	char* ID;
	uint16_t C;
	uint16_t T;

	uint16_t R;
	bool ran;
	uint16_t P;
} PeriodicTask;

typedef struct {
	uint8_t rIndex;
	char* ID;
	uint16_t C;
	uint16_t r;
} AperiodicTask;

typedef struct {
	FILE* fout;
	
	char* tableBuff;
	uint8_t tableWidth;
	
	uint16_t time;
	uint8_t utilization;
	
	uint8_t taskCount; // Simulation.pCount + Simulation.aCount
	uint8_t* pCount; // preemption count lookup-table of length taskCount
	uint8_t* dCount; // missed deadlines lookup-table of length taskCount
} SimReporter;

typedef struct {
	SimReporter reporter;
	
	uint16_t time;
	uint8_t pCount;
	uint8_t aCount;
	
	PeriodicTask* pTasks;
	AperiodicTask* aTasks;
} Simulation;

// Operates on an iterative indirect current
// In other words: modifies the caller's value of "current" (sorry I was lazy, not like this is a library)
// Fills the buffer with a given id text truncated to 6 digits and centered
// Includes spaces on either side and a right side column separator
inline void colHeader(char* &current, char* id) {
	int len = strlen(id);
	
	// Left side space
	*current = ' ';
	++current;
	
	if (len >= 6) {
		// The (potentially) truncated id
		memcpy(current, id, 6);
	}
	else {
		// spaces prepended to the id
		int lhs = (6 - len) / 2;
		for (int j = 0; j < lhs; ++j) {
			current[j] = ' ';
		}
		
		// The centered id
		memcpy(current + lhs, id, len);
		
		// spaces appended to the id
		for (int j = lhs + len; j < 6; ++j) {
			current[j] = ' ';
		}
	}
	current = current + 6;
	
	// Right side space and column edge
	memcpy(current, " |", 2);
	current = current + 2;
}

// Does not modify indirect (caller's "current")
// Has a 4 digit limit on numbers it can print to the column
inline void colCounter(char* col, uint16_t n) {
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

// Assumes column borders are preserved
// Sets the time and empty spaces in each column
inline void clearRow(char* buff, uint16_t time, uint8_t cols) {
	// Print the time
	colCounter(buff + 1, time);
	
	// Empty spaces in each column
	for (int i = 0; i < cols; ++i) {
		//          + 1 => Skip the left table edge
		//                 9 * => 8 digits plus a column edge
		//                        + 1 => skip the time column
		memcpy(buff + 1 + (9 * (i + 1)), "        ", 8);
	}
}

// Prints the table header and initializes the state variables for a new simulation
void reportInit(Simulation* plan) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	
	// Reset the counters
	reporter->time = 0;
	reporter->utilization = 0;
	for (int i = 0; i < reporter->taskCount; ++i) {
		reporter->pCount[i] = reporter->dCount[i] = 0;
	}
	
	// Generate the first row of the table: "+---+"
	buff[0] = buff[reporter->tableWidth - 1] = '+';
	for (int i = 1; i < reporter->tableWidth - 1; ++i) {
		buff[i] = '-';
	}
	fprintf(reporter->fout, "%s", buff);
	
	// Generate header row---------------------------------+
	// Generate the column header: "|  Time  |"
	memcpy(buff, "|  Time  | ", 10);
	
	// Iteration placeholder for generating column headers
	char* current = buff + 10;
	
	// Generate " TaskID |" for each pTask and aTask
	for (int i = 0; i < plan->pCount; ++i) {
		colHeader(current, plan->pTasks[i].ID);
	}
	for (int i = 0; i < plan->aCount; ++i) {
		colHeader(current, plan->aTasks[i].ID);
	}
	
	fprintf(reporter->fout, "%s", buff);
	// Generate header row---------------------------------+
	
	// Generate separator row: "|---|---|---|"
	for (int i = 0; i < reporter->tableWidth; ++i) {
		buff[i] = i % 9 == 0 ? '|' : '-';
	}
	fprintf(reporter->fout, "%s", buff);
	
	// Prepare the buffer to flush rows
	clearRow(buff, reporter->time, reporter->taskCount);
}

void reportExecution(Simulation* plan, uint8_t rId) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	++(reporter->utilization);
	
	// See `clearRow` for breakdown, +4 for the 5'th character in the column
	// Compiler should simplify the arithmetic to two operations, left more for clarity
	buff[1 + (9 * (rId + 1)) + 4] = '*';
}
void reportRelease(Simulation* plan, uint8_t rId) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	
	// See `clearRow` for breakdown, +3 for the 4'th character in the column
	// Compiler should simplify the arithmetic to two operations, left more for clarity
	buff[1 + (9 * (rId + 1)) + 3] = 'r';
}
void reportPreemption(Simulation* plan, uint8_t rId) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	++(reporter->pCount[rId]);
	
	// See `clearRow` for breakdown, +3 for the 4'th character in the column
	// Compiler should simplify the arithmetic to two operations, left more for clarity
	buff[1 + (9 * (rId + 1)) + 3] = 'p';
}
void reportDeadlineMissed(Simulation* plan, uint8_t rId) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	++(reporter->dCount[rId]);
	
	// See `clearRow` for breakdown, +3 for the 4'th character in the column
	// Compiler should simplify the arithmetic to two operations, left more for clarity
	buff[1 + (9 * (rId + 1)) + 3] = 'd';
}

void reportFlushRow(Simulation* plan) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	
	// Print the row
	fprintf(reporter->fout, "%s", buff);
	
	// Increment time and reset the buffer
	clearRow(buff, ++(reporter->time), reporter->taskCount);
}

void reportFlushAll(Simulation* plan) {
	SimReporter* reporter = &(plan->reporter);
	char* buff = reporter->tableBuff;
	
	// Generate separator row: "|---|---|---|"
	for (int i = 0; i < reporter->tableWidth; ++i) {
		buff[i] = i % 9 == 0 ? '|' : '-';
	}
	fprintf(reporter->fout, "%s", buff);
	
	// Generate dCount stats row---------------------------+
	memcpy(buff + 1, " dCount ", 8);
	
	for (int i = 0; i < reporter->taskCount; ++i) {
		// See `clearRow` for breakdown
		colCounter(buff + 1 + (9 * (i + 1)), reporter->dCount[i]);
	}
	
	fprintf(reporter->fout, "%s", buff);
	// Generate dCount stats row---------------------------+
	
	// Generate pCount stats row---------------------------+
	memcpy(buff + 1, " pCount ", 8);
	
	for (int i = 0; i < reporter->taskCount; ++i) {
		// See `clearRow` for breakdown
		colCounter(buff + 1 + (9 * (i + 1)), reporter->pCount[i]);
	}
	
	fprintf(reporter->fout, "%s", buff);
	// Generate pCount stats row---------------------------+
	
	// Generate the last row of the table: "+---+"
	buff[0] = buff[reporter->tableWidth - 1] = '+';
	for (int i = 1; i < reporter->tableWidth - 1; ++i) {
		buff[i] = '-';
	}
	fprintf(reporter->fout, "%s", buff);
	
	int dCount = 0;
	int pCount = 0;
	
	for (int i = 0; i < reporter->taskCount; ++i) {
		dCount += reporter->dCount[i];
		pCount += reporter->pCount[i];
	}
	
	// Print the summary statistics
	fprintf(
		reporter->fout,
		"Utilization: %.4f\r\nMissed Deadlines: %i\r\nPreemption Count: %i\r\n",
		((float)reporter->utilization) / reporter->time,
		dCount,
		pCount);
}

void sortTasks(PeriodicTask** tasks, uint8_t pCount) {
	//put tasks in order by priority
	for (int i = 0; i < pCount; i++) {
		for (int j = i+1; j < pCount; j++) {
			if (tasks[j]->T < tasks[i]->T) {
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j]->T == tasks[i]->T) {
				if (tasks[j]->C > tasks[i]->C) {
					PeriodicTask* temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

uint8_t checkToRun(PeriodicTask** periodicTasks, uint8_t pCount) {
	for (int i = 0; i < pCount; i++) {
		if (!periodicTasks[i]->ran) {
			return i;
		}
	}
	return pCount;
}

void checkReleases(PeriodicTask** periodicTasks, uint8_t pCount, int msec, Simulation* plan) {
	for (int i = 0; i < pCount; i++) {
		periodicTasks[i]->P = periodicTasks[i]->T - ((msec+1)%periodicTasks[i]->T);
		if (periodicTasks[i]->P == periodicTasks[i]->T) {
			if (periodicTasks[i]->R != periodicTasks[i]->C) {
				reportPreemption(plan, periodicTasks[i]->rIndex);
				
				periodicTasks[i]->R = periodicTasks[i]->C;
			}
			periodicTasks[i]->ran = false;
		}
	}
}

void computePriority(PeriodicTask** tasks, uint8_t pCount) {
	for (int i = 0; i < pCount; i++) {
		for (int j = i+1; j < pCount; j++) {
			if (tasks[j]->P < tasks[i]->P && !tasks[i]->ran) {
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j]->P == tasks[i]->P) {
				if (tasks[j]->R <= tasks[i]->R) {
					PeriodicTask* temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

void RMSchedule(Simulation* plan) {
	PeriodicTask** task_set = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++) {
		//the task runtime
		plan->pTasks[i].R = plan->pTasks[i].C;
		//bool to help prioritize
		plan->pTasks[i].ran = false;
		task_set[i] = plan->pTasks + i;
	}
	sortTasks(task_set, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->time; i++) {
		running = checkToRun(task_set, plan->pCount);
		if (running < plan->pCount) {
			reportExecution(plan, task_set[running]->rIndex);
			
			task_set[running]->R--;
			if (task_set[running]->R == 0) {
				task_set[running]->ran = true;
				task_set[running]->R = task_set[running]->C;
			}
			if (previous != plan->pCount && task_set[running]->ID != task_set[previous]->ID &&
				task_set[previous]->R != task_set[previous]->C) {
				reportPreemption(plan, task_set[running]->rIndex);
			}
		}
		reportFlushRow(plan);
		checkReleases(task_set, plan->pCount, i, plan);
		previous = running;
	}
	free(task_set);
}

void EDFSchedule(Simulation* plan) {
	PeriodicTask** task_set = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++) {
		//the task runtime
		plan->pTasks[i].R = plan->pTasks[i].C;
		//bool to help prioritize
		plan->pTasks[i].ran = false;
		task_set[i] = plan->pTasks + i;
	}
	sortTasks(task_set, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->time; i++) {
		running = checkToRun(task_set, plan->pCount);
		if (running < plan->pCount) {
			reportExecution(plan, task_set[running]->rIndex);
			
			task_set[running]->R--;
			if (task_set[running]->R == 0) {
				task_set[running]->ran = true;
				task_set[running]->R = task_set[running]->C;
			}
			if (previous != plan->pCount && task_set[running]->ID != task_set[previous]->ID &&
				task_set[previous]->R != task_set[previous]->C ) {
				reportPreemption(plan, task_set[running]->rIndex);
			}
		}
		reportFlushRow(plan);
		previous = running;
		checkReleases(task_set, plan->pCount, i, plan);
		computePriority(task_set, plan->pCount);
	}
	free(task_set);
}

void Simulate(Simulation* plan) {
	fprintf(plan->reporter.fout, "------------- Test Table Output --------------\r\n");
	reportInit(plan);
	reportRelease(plan, 0); reportFlushRow(plan);
	reportRelease(plan, 1); reportFlushRow(plan);
	reportRelease(plan, 2); reportFlushRow(plan);
	reportRelease(plan, 3); reportFlushRow(plan);
	reportRelease(plan, 4); reportFlushRow(plan);
	reportRelease(plan, 5); reportFlushRow(plan);
	reportRelease(plan, 6); reportFlushRow(plan);
	reportRelease(plan, 7); reportFlushRow(plan);
	reportRelease(plan, 8); reportFlushRow(plan);
	reportRelease(plan, 9); reportFlushRow(plan);
	reportPreemption(plan, 0); reportFlushRow(plan);
	reportPreemption(plan, 1); reportFlushRow(plan);
	reportPreemption(plan, 2); reportFlushRow(plan);
	reportPreemption(plan, 3); reportFlushRow(plan);
	reportPreemption(plan, 4); reportFlushRow(plan);
	reportPreemption(plan, 5); reportFlushRow(plan);
	reportPreemption(plan, 6); reportFlushRow(plan);
	reportPreemption(plan, 7); reportFlushRow(plan);
	reportPreemption(plan, 8); reportFlushRow(plan);
	reportPreemption(plan, 9); reportFlushRow(plan);
	reportDeadlineMissed(plan, 0); reportFlushRow(plan);
	reportDeadlineMissed(plan, 1); reportFlushRow(plan);
	reportDeadlineMissed(plan, 2); reportFlushRow(plan);
	reportDeadlineMissed(plan, 3); reportFlushRow(plan);
	reportDeadlineMissed(plan, 4); reportFlushRow(plan);
	reportDeadlineMissed(plan, 5); reportFlushRow(plan);
	reportDeadlineMissed(plan, 6); reportFlushRow(plan);
	reportDeadlineMissed(plan, 7); reportFlushRow(plan);
	reportDeadlineMissed(plan, 8); reportFlushRow(plan);
	reportDeadlineMissed(plan, 9); reportFlushRow(plan);
	reportExecution(plan, 0); reportFlushRow(plan);
	reportExecution(plan, 1); reportFlushRow(plan);
	reportExecution(plan, 2); reportFlushRow(plan);
	reportExecution(plan, 3); reportFlushRow(plan);
	reportExecution(plan, 4); reportFlushRow(plan);
	reportExecution(plan, 5); reportFlushRow(plan);
	reportExecution(plan, 6); reportFlushRow(plan);
	reportExecution(plan, 7); reportFlushRow(plan);
	reportExecution(plan, 8); reportFlushRow(plan);
	reportExecution(plan, 9); reportFlushRow(plan);
	reportRelease(plan, 0); reportExecution(plan, 0); reportFlushRow(plan);
	reportRelease(plan, 1); reportExecution(plan, 1); reportFlushRow(plan);
	reportRelease(plan, 2); reportExecution(plan, 2); reportFlushRow(plan);
	reportRelease(plan, 3); reportExecution(plan, 3); reportFlushRow(plan);
	reportRelease(plan, 4); reportExecution(plan, 4); reportFlushRow(plan);
	reportRelease(plan, 5); reportExecution(plan, 5); reportFlushRow(plan);
	reportRelease(plan, 6); reportExecution(plan, 6); reportFlushRow(plan);
	reportRelease(plan, 7); reportExecution(plan, 7); reportFlushRow(plan);
	reportRelease(plan, 8); reportExecution(plan, 8); reportFlushRow(plan);
	reportRelease(plan, 9); reportExecution(plan, 9); reportFlushRow(plan);
	reportPreemption(plan, 0); reportExecution(plan, 0); reportFlushRow(plan);
	reportPreemption(plan, 1); reportExecution(plan, 1); reportFlushRow(plan);
	reportPreemption(plan, 2); reportExecution(plan, 2); reportFlushRow(plan);
	reportPreemption(plan, 3); reportExecution(plan, 3); reportFlushRow(plan);
	reportPreemption(plan, 4); reportExecution(plan, 4); reportFlushRow(plan);
	reportPreemption(plan, 5); reportExecution(plan, 5); reportFlushRow(plan);
	reportPreemption(plan, 6); reportExecution(plan, 6); reportFlushRow(plan);
	reportPreemption(plan, 7); reportExecution(plan, 7); reportFlushRow(plan);
	reportPreemption(plan, 8); reportExecution(plan, 8); reportFlushRow(plan);
	reportPreemption(plan, 9); reportExecution(plan, 9); reportFlushRow(plan);
	reportDeadlineMissed(plan, 0); reportExecution(plan, 0); reportFlushRow(plan);
	reportDeadlineMissed(plan, 1); reportExecution(plan, 1); reportFlushRow(plan);
	reportDeadlineMissed(plan, 2); reportExecution(plan, 2); reportFlushRow(plan);
	reportDeadlineMissed(plan, 3); reportExecution(plan, 3); reportFlushRow(plan);
	reportDeadlineMissed(plan, 4); reportExecution(plan, 4); reportFlushRow(plan);
	reportDeadlineMissed(plan, 5); reportExecution(plan, 5); reportFlushRow(plan);
	reportDeadlineMissed(plan, 6); reportExecution(plan, 6); reportFlushRow(plan);
	reportDeadlineMissed(plan, 7); reportExecution(plan, 7); reportFlushRow(plan);
	reportDeadlineMissed(plan, 8); reportExecution(plan, 8); reportFlushRow(plan);
	reportDeadlineMissed(plan, 9); reportExecution(plan, 9); reportFlushRow(plan);
	reportFlushAll(plan);
	fprintf(plan->reporter.fout, "\r\n");
	
	fprintf(plan->reporter.fout, "------------- Rate Monotonic --------------\r\n");
	reportInit(plan);
	RMSchedule(plan);
	reportFlushAll(plan);
	fprintf(plan->reporter.fout, "\r\n");
	
	fprintf(plan->reporter.fout, "------------- Earliest Deadline First --------------\r\n");
	reportInit(plan);
	EDFSchedule(plan);
	reportFlushAll(plan);
}

void ParseFile(Simulation* plan, FILE* fin) {
	size_t buffsize = 128;
	char* buff = (char*)calloc(sizeof(char), buffsize);

	// Parse the file to get pCount
	fgets(buff, buffsize, fin);
	plan->pCount = atoi(buff);

	// Parse the file to get time
	fgets(buff, buffsize, fin);
	plan->time = atoi(buff);
	
	printf("Time: %i\npCount: %i\n", plan->time, plan->pCount);
	
	plan->pTasks = (PeriodicTask*)calloc(sizeof(PeriodicTask), plan->pCount);
	
	size_t line_n; // n is the sizeof each read line including 2 for \r\n
	size_t id_n; // a counter used to get the size of a task id
	
	// Parse the file pCount times to get the data for each periodic task
	for(int i = 0; i < plan->pCount; ++i) {
		line_n = getline(&buff, &buffsize, fin);
		
		PeriodicTask* task = (plan->pTasks) + i;
		
		// Set the index for the output table
		task->rIndex = i;
		
		// Get the ID
		int eos = 0; // end of string
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		// calloc for convenient null terminator
		// performance implications compared to manually adding \0 after malloc and memcpy unknown
		task->ID = (char*)calloc(sizeof(char), eos + 1);
		memcpy(task->ID, buff, eos);

		// Get the execution time
		int bos = eos++; // beginning of string
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		buff[eos] = 0; // create null pointer to aid atoi
		task->C = atoi(buff + bos);

		// Get the period
		bos = eos++;
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		buff[eos] = 0; // create null pointer to aid atoi
		task->T = atoi(buff + bos);

		printf("pTasks[%i]: {ID: \"%s\", C: %i, T: %i}\n", i, task->ID, task->C, task->T);
	}
	
	// Parse the file to get aCount
	fgets(buff, buffsize, fin);
	plan->aCount = atoi(buff);
	
	printf("aCount: %i\n", plan->aCount);
	
	plan->aTasks = (AperiodicTask*)calloc(sizeof(AperiodicTask), plan->aCount);
	
	// Parse the file aCount times to get the data for each periodic task
	for(int i = 0; i < plan->pCount; ++i) {
		line_n = getline(&buff, &buffsize, fin);
		
		AperiodicTask* task = (plan->aTasks) + i;
		
		// Set the index for the output table
		task->rIndex = plan->pCount + i;
		
		// Get the ID
		int eos = 0; // end of string
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		// calloc for convenient null terminator
		// performance implications compared to manually adding \0 after malloc and memcpy unknown
		task->ID = (char*)calloc(sizeof(char), eos + 1);
		memcpy(task->ID, buff, eos);
		
		// Get the execution time
		int bos = eos++; // beginning of string
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		buff[eos] = 0; // create null pointer to aid atoi
		task->C = atoi(buff + bos);
		
		// Get the absolute release time
		bos = eos++;
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		buff[eos] = 0; // create null pointer to aid atoi
		task->r = atoi(buff + bos);
		
		printf("aTasks[%i]: {ID: \"%s\", C: %i, r: %i}\n", i, task->ID, task->C, task->r);
	}
}

int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	Simulation* plan = (Simulation*)calloc(sizeof(Simulation), 1);
	SimReporter* reporter = &(plan->reporter);
	
	// Parse the input file
	FILE* fin = fopen(filein, "r");
	ParseFile(plan, fin);
	fclose(fin);
	
	// Initialize reporter-----------------------------------------------------------------+
	reporter->taskCount = plan->pCount + plan->aCount;
	//                      9 => 8-char wide column plus one edge of the column border
	//                                               + 1 for the time column
	//                                                     + 1 for the table edge
	reporter->tableWidth = (9 * (reporter->taskCount + 1)) + 1;
	
	// Add the \r\n\0 to each line without the reporter being "conscious" of it
	reporter->tableBuff = (char*)malloc(reporter->tableWidth + 3);
	memcpy(reporter->tableBuff + reporter->tableWidth, "\r\n\0", 3);
	
	// Initialize the counter arrays
	reporter->pCount = (uint8_t*)calloc(sizeof(uint8_t), reporter->taskCount);
	reporter->dCount = (uint8_t*)calloc(sizeof(uint8_t), reporter->taskCount);
	// Initialize reporter-----------------------------------------------------------------+
	
	// Run the simulation
	reporter->fout = fopen(fileout, "w");
	Simulate(plan);
	fclose(reporter->fout);
	
	// Cleanup reporter
	free(reporter->tableBuff);
	free(reporter->pCount);
	free(reporter->dCount);
	
	// Cleanup pTasks
	for (int i = 0; i < plan->pCount; ++i) {
		free(plan->pTasks[i].ID);
	}
	free(plan->pTasks);
	
	// Cleanup aTasks
	for (int i = 0; i < plan->aCount; ++i) {
		free(plan->aTasks[i].ID);
	}
	free(plan->aTasks);
	
	// Cleanup plan
	free(plan);
	
	return 0;
}
