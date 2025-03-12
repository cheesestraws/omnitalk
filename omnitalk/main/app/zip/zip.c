#include "zip.h"
#include "zip_internal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "proto/zip.h"
#include "table/routing/route.h"
#include "table/routing/table.h"
#include "table/zip/table.h"
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

static void zip_send_requests_if_necessary(rt_route_t *route) {
	buffer_t *buff;

	if (!zt_contains_net(global_zip_table, route->range_start)) {
		ESP_LOGE(TAG, "saw network %d but this isn't in the ZIP table; probably a bug", route->range_start);
		return;
	}
	
	if (zt_get_network_complete(global_zip_table, route->range_start)) {
		// We've already got the zones for this network, ignore this.
		return;
	}
	
	// This is a network we don't have all the zones for yet, let's ask for them
	buff = newbuf_ddp();
	zip_qry_setup_packet(buff, 1);
	zip_qry_set_network(buff, 0, route->range_start);
	
	// todo: send packet here
	freebuf(buff);
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
				zt_add_net_range(global_zip_table, cmd->route.range_start, cmd->route.range_end);
				zip_send_requests_if_necessary(&cmd->route);
				printf("zip table:\n"); zt_print(global_zip_table);
				break;
			case ZIP_NETWORK_DELETED:
				zt_delete_network(global_zip_table, cmd->route.range_start);
				printf("zip table:\n"); zt_print(global_zip_table);
				break;
		}
		free(cmd);
	}
}

void app_zip_start(void) {
	global_zip_table = zt_new();
}
