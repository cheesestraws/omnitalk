#include "runloop.h"

bool rlsend(runloop_info_t *rl, buffer_t *buf) {
	BaseType_t err = xQueueSendToBack(rl->incoming_packet_queue,
		&buf, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}
