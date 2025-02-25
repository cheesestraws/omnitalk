#include "controlplane_runloop.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <stdio.h>

#include "mem/buffers.h"

static void controlplane_runloop(void* dummy) {
	vTaskDelay(portMAX_DELAY);
}


runloop_info_t start_controlplane_runloop(void) {
	runloop_info_t info = {0};
	
	info.incoming_packet_queue = xQueueCreate(CONTROLPLANE_QUEUE_DEPTH, sizeof(buffer_t*));;
	xTaskCreate(&controlplane_runloop, "CTRL", 8192, NULL, 5, &info.task);

	printf("start control plane\n");
	return info;
}
