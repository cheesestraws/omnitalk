#include "zip.h"
#include "zip_internal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "lap/lap.h"
#include "proto/zip.h"
#include "table/routing/route.h"
#include "table/routing/table.h"
#include "table/zip/table.h"
#include "web/stats.h"
#include "ddp_send.h"
#include "global_state.h"

const static char* TAG = "ZIP";

static void propagate_zone_to_lap(zip_zone_tuple_t *t) {
	// If this is a zone reply for a directly-connected network, we pick the first
	// zone for that network and populate the zone name associated with the LAP with it.
	rt_route_t route = { 0 };
	
	bool found = rt_lookup(global_routing_table, ZIP_TUPLE_NETWORK(t), &route);
	if (!found) {
		ESP_LOGW(TAG, "no route for network appearing in ZIP tuple: %d", ZIP_TUPLE_NETWORK(t));
		return;
	}
	
	if (route.distance == 0) {
		// Direct route!
		if (route.outbound_lap != NULL && route.outbound_lap->my_zone == NULL) {	
			// We will never change this again, so we can allocate it and will never
			// free it.
			char* zone = pstring_to_cstring_alloc(&ZIP_TUPLE_ZONE_NAME(t));
			lap_set_my_zone(route.outbound_lap, zone);
			lap_registry_update_zone_cache(global_lap_registry);
		}
	}
}

static void app_zip_handle_extended_reply(buffer_t *packet) {
	stats.zip_in_replies__kind_extended++;
	
	int count = (int)zip_packet_get_network_count(packet);
	
	bool first_tuple = true;
	// An extended reply is always about a single network
	uint16_t relevant_network = 0;
	
	// Iterate through the tuples, adding them to the network
	zip_zone_tuple_t *t;
	char* zone_cstr;
	
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		if (first_tuple) {
			zt_set_expected_zone_count(global_zip_table, ZIP_TUPLE_NETWORK(t), count);
			relevant_network = ZIP_TUPLE_NETWORK(t);
			
			propagate_zone_to_lap(t);
		}
	
		zone_cstr = pstring_to_cstring_alloc(&ZIP_TUPLE_ZONE_NAME(t));
		zt_add_zone_for(global_zip_table, ZIP_TUPLE_NETWORK(t), zone_cstr);
		free(zone_cstr);
		first_tuple = false;
	}
	
	zt_check_zone_count_for_completeness(global_zip_table, relevant_network);
}

static void app_zip_handle_nonextended_reply(buffer_t *packet) {
	stats.zip_in_replies__kind_nonextended++;

	// Iterate through the tuples, adding them to the networks
	zip_zone_tuple_t *t;
	char* zone_cstr;
	bool first_tuple = true;
	
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		if (first_tuple) {
			propagate_zone_to_lap(t);
		}
		
		zone_cstr = pstring_to_cstring_alloc(&ZIP_TUPLE_ZONE_NAME(t));
		zt_add_zone_for(global_zip_table, ZIP_TUPLE_NETWORK(t), zone_cstr);
		free(zone_cstr);
		
		first_tuple = false;
	}
	
	// Then mark each network entry as complete
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		zt_mark_network_complete(global_zip_table, ZIP_TUPLE_NETWORK(t));
	}
}

void app_zip_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) == 6 && 
	    packet->ddp_payload_length > sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_REPLY) {
	
		app_zip_handle_nonextended_reply(packet);
	} else if (DDP_TYPE(packet) == 6 && 
	    packet->ddp_payload_length > sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_EXTENDED_REPLY) {
	
		app_zip_handle_extended_reply(packet);
	}

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
	
	// If the route is a direct route, we need to send this as a broadcast
	// via the LAP it came in on.
	bool buffer_no_longer_our_problem;
	if (route->distance == 0) {
		buffer_no_longer_our_problem = ddp_send_via(buff, DDP_SOCKET_ZIP,
			0, 255, DDP_SOCKET_ZIP, 6, route->outbound_lap);
	} else {
		// If it's a distant route, we send this as a unicast to the corresponding
		// nexthop.
		buffer_no_longer_our_problem = ddp_send(buff, DDP_SOCKET_ZIP,
			route->nexthop.network, route->nexthop.node, DDP_SOCKET_ZIP, 6);
	}
	
	if (!buffer_no_longer_our_problem) {
		stats.zip_out_errors__err_ddp_send_failed++;
		freebuf(buff);
	} else {
		stats.zip_out_queries++;
	}
}

void app_zip_idle(void*) {
	zip_internal_command_t* cmd = NULL;
	
	// Initialise stuff
	zip_cmd_queue = xQueueCreate(20, sizeof(zip_internal_command_t*));
	rt_attach_touch_callback(global_routing_table, &route_touched_callback);
	rt_attach_net_range_removed_callback(global_routing_table, &network_deleted_callback);
	
	// Loop and execute commands
	while (1) {
		BaseType_t ret = xQueueReceive(zip_cmd_queue, &cmd, 20000 / portTICK_PERIOD_MS);
		if (ret == pdTRUE) {
			switch (cmd->cmd) {
				case ZIP_NETWORK_TOUCHED:
					zt_add_net_range(global_zip_table, cmd->route.range_start, cmd->route.range_end);
					zip_send_requests_if_necessary(&cmd->route);
					break;
				case ZIP_NETWORK_DELETED:
					zt_delete_network(global_zip_table, cmd->route.range_start);
					break;
			}
			free(cmd);
		}
		// emit stats
		char* new_stats = zt_stats(global_zip_table);
		char* old_stats = atomic_exchange(&stats_zip_table, new_stats);
		if (old_stats != NULL) {
			free(old_stats);
		}
	}
}

void app_zip_start(void) {
	global_zip_table = zt_new();
}
