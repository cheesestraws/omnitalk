#include "lap.h"

#include "web/stats.h"

bool lsend(lap_t *lap, buffer_t *buff) {
	BaseType_t err = xQueueSendToBack(lap->outbound,
		&buff, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}

void lap_set_my_zone(lap_t *lap, pstring* zone) {
	lap->my_zone = zone;
	stats_lap_metadata[lap->id].zone = pstring_to_cstring_alloc(zone);
}
