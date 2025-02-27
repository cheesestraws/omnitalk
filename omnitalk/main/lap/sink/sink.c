#include "lap/sink/sink.h"

#include <stdio.h>

#include <esp_err.h>
#include <esp_log.h>

#include "mem/buffers.h"

static const char* TAG="SINK";

static void sink_rx_runloop(void* tp) {
	buffer_t *buff = NULL;
	transport_t *transport = (transport_t*)tp;
	ESP_ERROR_CHECK(enable_transport(transport));
	ESP_LOGI(TAG, "started sink for %s", transport->kind);
	while(1) {
		xQueueReceive(transport->inbound, &buff, portMAX_DELAY);
		if (buff != NULL) {
			//ESP_LOGI(TAG, "sink %s: received frame of %zu bytes", transport->kind, buff->length);
			freebuf(buff);
		}
	}
}


esp_err_t start_sink(char* name, transport_t* transport) {
	TaskHandle_t task;
	if (xTaskCreate(&sink_rx_runloop, name, 2048, (void*)transport, 5, &task)) {
		return ESP_OK;
	}
	
	return ESP_FAIL;
}
