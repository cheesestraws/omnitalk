#include "controlplane_runloop.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <stdbool.h>
#include <stdio.h>

#include "app/app.h"
#include "mem/buffers.h"
#include "proto/ddp.h"

static const char* TAG = "CTRL";
static QueueHandle_t inbound;

static void controlplane_runloop(void* dummy) {
	buffer_t *packet;
	while(1) {
		// receive a packet
		xQueueReceive(inbound, &packet, portMAX_DELAY);
		if (packet == NULL) {
			continue;
		}
		
		if (!packet->ddp_ready) {
			goto cleanup;
		}
		
		// dispatch it to an application.  For the moment we pretend everything is
		// unicast
		uint8_t dstsock = DDP_DSTSOCK(packet);
		bool handled = false;
		for (int i = 0; unicast_apps[i].socket_number != 0; i++) {
			if (unicast_apps[i].socket_number == dstsock) {
				unicast_apps[i].handler(packet);
				handled = true;
				break;
			}
		}
		if (!handled) {
			goto cleanup;
		}
		
		
		// free it otherwise
		continue;
	cleanup:
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
