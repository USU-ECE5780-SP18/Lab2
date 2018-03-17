#include "parser.h"
#include "reporter.h"
#include <stdlib.h>

Schedule* RmSimulation(SimPlan* plan) {
	Schedule* sched = MakeSchedule(plan);

	return sched;
}
