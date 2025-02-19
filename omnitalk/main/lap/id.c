#include "lap/id.h"

#include <assert.h>
#include <stdatomic.h>

int get_next_lap_id(void) {
	static _Atomic int id = 0;
	
	int next_id = ++id;
	
	assert(next_id < MAX_LAP_COUNT);
	
	return next_id;
}
