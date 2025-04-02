#include "zip.h"
#include "zip_internal.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "lap/lap.h"
#include "proto/atp.h"
#include "proto/ddp.h"
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
			pstring* zone = pstrclone(&ZIP_TUPLE_ZONE_NAME(t));
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
	
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		if (first_tuple) {
			zt_set_expected_zone_count(global_zip_table, ZIP_TUPLE_NETWORK(t), count);
			relevant_network = ZIP_TUPLE_NETWORK(t);
			
			propagate_zone_to_lap(t);
		}
	
		zt_add_zone_for(global_zip_table, ZIP_TUPLE_NETWORK(t), &ZIP_TUPLE_ZONE_NAME(t));
		first_tuple = false;
	}
	
	zt_check_zone_count_for_completeness(global_zip_table, relevant_network);
}

static void app_zip_handle_nonextended_reply(buffer_t *packet) {
	stats.zip_in_replies__kind_nonextended++;

	// Iterate through the tuples, adding them to the networks
	zip_zone_tuple_t *t;
	bool first_tuple = true;
	
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		if (first_tuple) {
			propagate_zone_to_lap(t);
		}
		zt_add_zone_for(global_zip_table, ZIP_TUPLE_NETWORK(t), &ZIP_TUPLE_ZONE_NAME(t));		
		first_tuple = false;
	}
	
	// Then mark each network entry as complete
	for (t = zip_reply_get_first_tuple(packet); t != NULL; t = zip_reply_get_next_tuple(packet, t)) {
		zt_mark_network_complete(global_zip_table, ZIP_TUPLE_NETWORK(t));
	}
}

/* ZIP query handling */

struct zip_query_response_state {
	uint8_t to_node;
	uint16_t to_net;
	uint8_t to_socket;
	
	size_t zone_count;
	
	buffer_t *packet_in_progress;	
};

// These three functions form a loop iterating over the zone table
static bool query_reply_iterator_init(void* pvt, uint16_t network, bool exists, size_t zone_count, bool complete) {
	struct zip_query_response_state *state = (struct zip_query_response_state*)pvt;

	// If this network doesn't exist or the zone records aren't complete, bail out.
	if (!exists) {
		return false;
	}
	if (!complete) {
		return false;
	}
	if (zone_count == 0) {
		return false;
	}
	
	state->zone_count = zone_count;
	return true;
}

static bool query_reply_iterator_loop(void* pvt, int idx, uint16_t network, pstring* zone) {
	struct zip_query_response_state *state = (struct zip_query_response_state*)pvt;
	bool try_again = false;
	
	do {
		// Do we need to create a new packet buffer?
		if (state->packet_in_progress == NULL) {
			state->packet_in_progress = newbuf_ddp();
			buf_expand_payload(state->packet_in_progress, sizeof(zip_packet_t));
			zip_ext_reply_setup_packet(state->packet_in_progress, state->zone_count);
		}
	
		// Do we have room in the current packet for this next tuple?
		size_t tuple_len = zone->length + 3;
		if (state->packet_in_progress->ddp_payload_length + tuple_len <= DDP_MAX_PAYLOAD_LEN) {
			// Happy days!
			buf_append_uint16(state->packet_in_progress, network);
			buf_append_pstring(state->packet_in_progress, zone);
			try_again = false;
		} else {
			// Send this packet off and try again with a fresh one
			if (!ddp_send(state->packet_in_progress, DDP_SOCKET_ZIP, state->to_net,
			              state->to_node, state->to_socket, 6)) {
				stats.zip_out_errors__err_ddp_send_failed++;
				free(state->packet_in_progress);
			} else {
				stats.zip_out_replies__kind_extended++;
			}
			state->packet_in_progress = NULL;
			try_again = true;
		}
	} while(try_again);
	
	return true;
}

static bool query_reply_iterator_end(void* pvt, bool aborted) {
 	struct zip_query_response_state *state = (struct zip_query_response_state*)pvt;
 
 	if (aborted) {
 		return false;
 	}
 	
 	if (state->packet_in_progress != NULL) {
 		// Send our last chunk of reply
		if (!ddp_send(state->packet_in_progress, DDP_SOCKET_ZIP, state->to_net,
					  state->to_node, state->to_socket, 6)) {
			stats.zip_out_errors__err_ddp_send_failed++;
			free(state->packet_in_progress);
		} else {
			stats.zip_out_replies__kind_extended++;
		}
		state->packet_in_progress = NULL;
 	}
 	
 	return true;
 }

static void app_zip_reply_to_query_for_net(buffer_t *packet, uint16_t net) {
	struct zip_query_response_state state = {
		.to_node = DDP_SRC(packet),
		.to_net = DDP_SRCNET(packet),
		.to_socket = DDP_SRCSOCK(packet)
	};
	
	zt_iterate_net(global_zip_table, &state, net,
		query_reply_iterator_init, query_reply_iterator_loop, query_reply_iterator_end);
}

void app_zip_handle_query(buffer_t *packet) {
	uint8_t network_count = zip_packet_get_network_count(packet);
	
	// Do we have the number of networks the header claims we have?
	int remaining_bytes_after_hdr = packet->ddp_payload_length - sizeof(zip_packet_t);
	if (remaining_bytes_after_hdr < network_count * 2) {
		stats.zip_in_errors__err_query_packet_too_short++;
		return;
	}
	
	for (int i = 0; i < network_count; i++) {
		uint16_t net = zip_qry_get_network(packet, i);
		app_zip_reply_to_query_for_net(packet, net);
	}
}

void app_zip_handle_atp(buffer_t *packet) {
	uint8_t *user_data = atp_packet_get_user_data(packet);
	uint8_t command = user_data[0];
	
	if (command == 7 || command == 8 || command == 9) {
		app_zip_handle_get_zone_list(packet);
	} else {
		stats.zip_in_errors__err_unknown_atp_packet_command++;
	}
	
}

void app_zip_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) == DDP_TYPE_ZIP && 
	    packet->ddp_payload_length >= sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_REPLY) {
	
		app_zip_handle_nonextended_reply(packet);
	} else if (DDP_TYPE(packet) == DDP_TYPE_ZIP && 
	    packet->ddp_payload_length >= sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_EXTENDED_REPLY) {
	
		app_zip_handle_extended_reply(packet);
	} else if (DDP_TYPE(packet) == DDP_TYPE_ZIP && 
	    packet->ddp_payload_length >= sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_QUERY) {
	    
		app_zip_handle_query(packet);
	} else if (DDP_TYPE(packet) == DDP_TYPE_ZIP && 
	    packet->ddp_payload_length >= sizeof(zip_packet_t) &&
	    ZIP_FUNCTION(packet) == ZIP_GETNETINFO) {
	    
	    app_zip_handle_get_net_info(packet);
	} else if (DDP_TYPE(packet) == DDP_TYPE_ATP &&
	    packet->ddp_payload_length >= sizeof(atp_packet_t)) {
	 	
	  app_zip_handle_atp(packet);
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
