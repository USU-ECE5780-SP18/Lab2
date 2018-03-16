#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char* ID;
	uint16_t C;
	uint16_t R;
	uint16_t T;
	bool ran;
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

void sortTasks(PeriodicTask* tasks, uint8_t pCount){
	//put tasks in order by priority
	for (int i = 0; i < pCount; i++){
		for (int j = i+1; j < pCount; j++){
			if (tasks[j].T < tasks[i].T){
				PeriodicTask temp = tasks[i];
				tasks[i] = tasks[j];
				tasks[j] = temp;
			} else if (tasks[j].T == tasks[i].T){
				if (tasks[j].C > tasks[i].C){
					PeriodicTask temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
			}
		}
	}
}

uint8_t checkToRun(PeriodicTask* periodicTasks, uint8_t pCount){
	for (int i = 0; i < pCount; i++){
		if (!periodicTasks[i].ran){
			return i;
		}
	}
	return pCount;
}

void checkReleases(PeriodicTask* periodicTasks, uint8_t pCount, int msec){
	for (int i = 0; i < pCount; i++){
		if (!((msec+1)%periodicTasks[i].T)) periodicTasks[i].ran = false;
	}
}

void Simulate(Simulation* plan) {
	//this is edf
//	PeriodicTask* task_set = (PeriodicTask*)calloc(sizeof(PeriodicTask), plan->pCount*sizeof(PeriodicTask));
	sortTasks(plan->pTasks, plan->pCount);
	uint8_t running;
	uint8_t previous = 0;
	for (int i = 0; i < plan->time; i++){
		running = checkToRun(plan->pTasks, plan->pCount);
		if (running < plan->pCount){
			fprintf(plan->fout, "%d : %s\n", i, plan->pTasks[running].ID);
			plan->pTasks[running].R--;
			if (plan->pTasks[running].R == 0){
				plan->pTasks[running].ran = true;
				plan->pTasks[running].R = plan->pTasks[running].C;
			}
			if (plan->pTasks[running].ID != plan->pTasks[previous].ID &&
					plan->pTasks[previous].R != plan->pTasks[previous].C &&
					previous != plan->pCount){
				fprintf(plan->fout, "%s was preempted.\n", plan->pTasks[previous].ID);
			}
		} else {
			fprintf(plan->fout, "%d : %s\n", i, "slack");
		}
		checkReleases(plan->pTasks, plan->pCount, i);
		previous = running;
	}
}

#define BUFF_N 256
int main(int argc, char** argv) {
	const char* filein = argv[1];
	const char* fileout = argv[2];
	
	printf("The input file: %s\nThe output file: %s\r\n", filein, fileout);
	
	Simulation* plan = (Simulation*)calloc(sizeof(Simulation), 1);
	
	FILE* fin = fopen(filein, "r");
	
	size_t buffsize = 456;
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
		printf("buff[%i]: %s", line_n, buff);
		
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
		//the task runtime
		task->R = task->C;
		
		// Get the period
		bos = eos++;
		while (eos < line_n && buff[eos] == ' ') { ++eos; ++bos; } // Get rid of space
		while (eos < line_n && buff[eos] != ',') { ++eos; }
		// A fully rigorous program would probably do some validation here
		buff[eos] = 0; // create null pointer to aid atoi
		task->T = atoi(buff + bos);

		//bool to help prioritize
		task->ran = false;
		
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
		printf("buff[%i]: %s", line_n, buff);
		
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
	
	// Cleanup the input file buffer
	fclose(fin);
	
	plan->fout = fopen(fileout, "w");
	
	// Run the simulation
	Simulate(plan);
	
	// Cleanup after ourselves
	fclose(plan->fout);

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
