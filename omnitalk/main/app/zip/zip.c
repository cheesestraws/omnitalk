#include "zip.h"
#include "zip_internal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "table/routing/route.h"
#include "table/routing/table.h"
#include "global_state.h"

const static char* TAG = "ZIP";

void app_zip_handler(buffer_t *packet) {
	freebuf(packet);
}

// zip_cmd_queue transfers events from event handlers attached to the routing table
// to the zip idle runloop
static QueueHandle_t zip_cmd_queue;

static void route_touched_callback(void* param) {
	if (zip_cmd_queue == NULL) {
		return;
	}
	
	if (param == NULL) {
		return;
	}
	
	zip_internal_command_t* cmd = calloc(1, sizeof(zip_internal_command_t));
	cmd->cmd = ZIP_NETWORK_TOUCHED;
	cmd->route = *(rt_route_t*)param;
	xQueueSendToBack(zip_cmd_queue, &cmd, 0);
}

static void network_deleted_callback(void* param) {
	if (zip_cmd_queue == NULL) {
		return;
	}
	
	if (param == NULL) {
		return;
	}
	
	zip_internal_command_t* cmd = calloc(1, sizeof(zip_internal_command_t));
	cmd->cmd = ZIP_NETWORK_DELETED;
	cmd->route = *(rt_route_t*)param;
	xQueueSendToBack(zip_cmd_queue, &cmd, 0);
}

void app_zip_idle(void*) {
	zip_internal_command_t* cmd;
	
	// Initialise stuff
	zip_cmd_queue = xQueueCreate(20, sizeof(zip_internal_command_t*));
	rt_attach_touch_callback(global_routing_table, &route_touched_callback);
	rt_attach_net_range_removed_callback(global_routing_table, &network_deleted_callback);
	
	// Loop and execute commands
	while (1) {
		xQueueReceive(zip_cmd_queue, &cmd, portMAX_DELAY);
		switch (cmd->cmd) {
			case ZIP_NETWORK_TOUCHED:
				break;
			case ZIP_NETWORK_DELETED:
				break;
		}
		free(cmd);
	}
}
