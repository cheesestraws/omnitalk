#include "util/event/event.h"

bool event_add_callback(event_t* event, event_callback_t callback) {
	if (event->callback_count == EVENT_MAX_CALLBACKS) {
		return false;
	}
	
	event->callbacks[event->callback_count] = callback;
	event->callback_count++;
	return true;
}

void event_fire(event_t* event, void* param) {
	for (int i = 0; i < event->callback_count; i++) {
		(event->callbacks[i])(param);
	}
}

