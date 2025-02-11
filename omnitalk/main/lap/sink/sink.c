#include "lap/sink/sink.h"

#include <stdio.h>

#include <esp_err.h>

#include "mem/buffers.h"

static void sink_rx_runloop(void* tp) {
	buffer_t *buff = NULL;
	transport_t *transport = (transport_t*)tp;
	ESP_ERROR_CHECK(enable_transport(transport));
	printf("started sink for %s\n", transport->kind);
	while(1) {
		xQueueReceive(transport->inbound, &buff, portMAX_DELAY);
		if (buff != NULL) {
			printf("sink %s: received frame of %zu bytes", transport->kind, buff->length);
			freebuf(buff);
		}
	}
}


esp_err_t start_sink(char* name, transport_t* transport) {
	TaskHandle_t task;
	if (xTaskCreate(&sink_rx_runloop, name, 1024, (void*)transport, 5, &task)) {
		return ESP_OK;
	}
	
	return ESP_FAIL;
}