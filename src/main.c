#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t T;

	uint16_t R;
	bool ran;
	uint16_t P;
} PeriodicTask;

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t r;
} AperiodicTask;

typedef struct {
	FILE* fout;
	uint16_t time;
	
	uint8_t pCount;
	uint8_t aCount;
	
	PeriodicTask* pTasks;
	AperiodicTask* aTasks;
} Simulation;

void sortTasks(PeriodicTask** tasks, uint8_t pCount){
	//put tasks in order by priority
	for (int i = 0; i < pCount; i++){
		for (int j = i+1; j < pCount; j++){
			if (tasks[j]->T < tasks[i]->T){
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j]->T == tasks[i]->T){
				if (tasks[j]->C > tasks[i]->C){
					PeriodicTask* temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

uint8_t checkToRun(PeriodicTask** periodicTasks, uint8_t pCount){
	for (int i = 0; i < pCount; i++){
		if (!periodicTasks[i]->ran){
			return i;
		}
	}
	return pCount;
}

void checkReleases(PeriodicTask** periodicTasks, uint8_t pCount, int msec, FILE* fout){
	for (int i = 0; i < pCount; i++){
		periodicTasks[i]->P = periodicTasks[i]->T - ((msec+1)%periodicTasks[i]->T);
		if (periodicTasks[i]->P == periodicTasks[i]->T){
			if (periodicTasks[i]->R != periodicTasks[i]->C){
				fprintf(fout, "%s has missed its deadline\n", periodicTasks[i]->ID);
				periodicTasks[i]->R = periodicTasks[i]->C;
			}
			periodicTasks[i]->ran = false;
		}
	}
}

void computePriority(PeriodicTask** tasks, uint8_t pCount){
	for (int i = 0; i < pCount; i++){
		for (int j = i+1; j < pCount; j++){
			if (tasks[j]->P < tasks[i]->P && !tasks[i]->ran){
				PeriodicTask* temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j]->P == tasks[i]->P){
				if (tasks[j]->R <= tasks[i]->R){
					PeriodicTask* temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

void RMSchedule(Simulation* plan){
	PeriodicTask** task_set = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++){
		//the task runtime
		plan->pTasks[i].R = plan->pTasks[i].C;
		//bool to help prioritize
		plan->pTasks[i].ran = false;
		task_set[i] = plan->pTasks + i;
	}
	sortTasks(task_set, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->time; i++){
		running = checkToRun(task_set, plan->pCount);
		if (running < plan->pCount){
			fprintf(plan->fout, "%d : %s\n", i, task_set[running]->ID);
			task_set[running]->R--;
			if (task_set[running]->R == 0){
				task_set[running]->ran = true;
				task_set[running]->R = task_set[running]->C;
			}
			if (previous != plan->pCount && task_set[running]->ID != task_set[previous]->ID &&
				task_set[previous]->R != task_set[previous]->C){
				fprintf(plan->fout, "%s was preempted.\n", task_set[previous]->ID);
			}
		} else {
			fprintf(plan->fout, "%d : %s\n", i, "slack");
		}
		checkReleases(task_set, plan->pCount, i, plan->fout);
		previous = running;
	}
	free(task_set);
}

void EDFSchedule(Simulation* plan){
	PeriodicTask** task_set = (PeriodicTask**)calloc(sizeof(PeriodicTask*), plan->pCount);
	for (int i = 0; i < plan->pCount; i++){
		//the task runtime
		plan->pTasks[i].R = plan->pTasks[i].C;
		//bool to help prioritize
		plan->pTasks[i].ran = false;
		task_set[i] = plan->pTasks + i;
	}
	sortTasks(task_set, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->time; i++){
		running = checkToRun(task_set, plan->pCount);
		if (running < plan->pCount){
			fprintf(plan->fout, "%d : %s\n", i, task_set[running]->ID);
			task_set[running]->R--;
			if (task_set[running]->R == 0){
				task_set[running]->ran = true;
				task_set[running]->R = task_set[running]->C;
			}
			if (previous != plan->pCount && task_set[running]->ID != task_set[previous]->ID &&
				task_set[previous]->R != task_set[previous]->C ){
				fprintf(plan->fout, "%s was preempted.\n", task_set[previous]->ID);
			}
		} else {
			fprintf(plan->fout, "%d : %s\n", i, "slack");
		}
		previous = running;
		checkReleases(task_set, plan->pCount, i, plan->fout);
		computePriority(task_set, plan->pCount);
	}
	free(task_set);
}

void Simulate(Simulation* plan) {
	fprintf(plan->fout, "------------- Rate Monotonic --------------\n");
	RMSchedule(plan);
	fprintf(plan->fout, "------------- Earliest Deadline First --------------\n");
	EDFSchedule(plan);
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
	
	// Parse the input file
	FILE* fin = fopen(filein, "r");
	ParseFile(plan, fin);
	fclose(fin);
	
	// Run the simulation
	plan->fout = fopen(fileout, "w");
	Simulate(plan);
	fclose(plan->fout);
	
	// Cleanup after ourselves
	for (int i = 0; i < plan->pCount; ++i) {
		free(plan->pTasks[i].ID);
	}
	for (int i = 0; i < plan->aCount; ++i) {
		free(plan->aTasks[i].ID);
	}
	free(plan->pTasks);
	free(plan->aTasks);
	free(plan);
	
	return 0;
}
