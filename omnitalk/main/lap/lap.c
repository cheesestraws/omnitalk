#include "lap.h"

bool lsend(lap_t *lap, buffer_t *buff) {
	BaseType_t err = xQueueSendToBack(lap->outbound,
		&buff, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}
