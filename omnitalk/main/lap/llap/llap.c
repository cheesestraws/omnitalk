#include "lap/llap/llap.h"

#include <stdlib.h>

#include "lap/lap.h"

void llap_inbound_runloop(void* lapParam) {
	vTaskDelay(portMAX_DELAY);
}

void llap_outbound_runloop(void* lapParam) {
	vTaskDelay(portMAX_DELAY);
}


lap_t *start_llap(char* name, transport_t *transport) {
	lap_t *lap = calloc(1, sizeof(lap_t));
	if (lap == NULL) {
		return NULL;
	}
	
	// fill in LAP fields
	lap->name = name;
	lap->transport = transport;
	
	// start runloop
	char* task_name;
	asprintf(&task_name, "llap:%s:inbound", name);
	xTaskCreate(&llap_inbound_runloop, task_name, 2048, (void*)lap, 5, NULL);
	// do NOT free task_name, freertos will be holding onto a reference to it
	asprintf(&task_name, "llap:%s:outbound", name);
	xTaskCreate(&llap_outbound_runloop, task_name, 2048, (void*)lap, 5, NULL);
	// freertos will hold onto a reference to this string, too

	return lap;
}