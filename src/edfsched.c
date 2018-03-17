#include "parser.h"
#include "reporter.h"
#include <stdlib.h>

Schedule* EdfSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	return sched;
}
