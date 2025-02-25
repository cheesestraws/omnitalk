#include "controlplane_runloop.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <stdio.h>

#include "mem/buffers.h"

static const char* TAG = "CTRL";
static QueueHandle_t inbound;

static void controlplane_runloop(void* dummy) {
	buffer_t *packet;
	while(1) {
		// receive a packet
		xQueueReceive(inbound, &packet, portMAX_DELAY);
		// dispatch it to an application
		ESP_LOGI(TAG, "control plane got a packet");
		// free it otherwise

		freebuf(packet);
	}

	vTaskDelay(portMAX_DELAY);
}


runloop_info_t start_controlplane_runloop(void) {
	runloop_info_t info = {0};
	
	inbound = xQueueCreate(CONTROLPLANE_QUEUE_DEPTH, sizeof(buffer_t*));
	info.incoming_packet_queue = inbound;
	xTaskCreate(&controlplane_runloop, "CTRL", 8192, NULL, 5, &info.task);

	printf("start control plane\n");
	return info;
}
