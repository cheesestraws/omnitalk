#include "app/rtmp/rtmp.h"

#include <stdatomic.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "proto/rtmp.h"
#include "table/routing/route.h"
#include "table/routing/table.h"
#include "web/stats.h"
#include "global_state.h"

static const char* TAG = "RTMP";

static void handle_rtmp_update_packet(buffer_t *packet) {
	stats.rtmp_update_packets++;
	
	// Is the packet long enough to be an actual DDP packet?
	if(DDP_BODYLEN(packet) < sizeof(rtmp_packet_t)) {
		stats.rtmp_errors__err_packet_too_short++;
		ESP_LOGE(TAG, "got too-short RTMP packet: %d", (int)(DDP_BODYLEN(packet)));
		return;
	}
	
	if (RTMP_ID_LEN(packet) != 8) {
		stats.rtmp_errors__err_wrong_id_len++;
		ESP_LOGE(TAG, "wrong ID length in RTMP packet: %d", (int)(RTMP_ID_LEN(packet)));
		return;		
	}
	
	if (packet->recv_chain.lap == NULL) {
		stats.rtmp_errors__err_invalid_lap++;
		ESP_LOGE(TAG, "NULL lap found when processing RTMP packet.  The whole router is probably about to fall over.");
		return;
	}
	
	lap_t *lap = packet->recv_chain.lap;
	
	// Is the router we're getting RTMP from even reachable?
	if (RTMP_ROUTER_NETWORK(packet) < lap->network_range_start || RTMP_ROUTER_NETWORK(packet) < lap->network_range_end) {
		stats.rtmp_errors__err_unreachable_nexthop++;
		return;
	}
		
	rtmp_tuple_t *cursor = NULL;
	
	for (cursor = get_first_rtmp_tuple(packet); cursor != NULL; cursor = get_next_rtmp_tuple(packet, cursor)) {
		rt_route_t route = {
			.range_start = RTMP_TUPLE_RANGE_START(cursor),
			.range_end = RTMP_TUPLE_RANGE_END(cursor),
	
			.outbound_lap = lap,
			.nexthop = {
				.network = RTMP_ROUTER_NETWORK(packet),
				.node = RTMP_ROUTER_NODE_ID(packet),
			},
		};
		
		// Distance 31 is code for "bad route", so we don't increment
		if (RTMP_TUPLE_DISTANCE(cursor) == 31) {
			route.distance = 31;
		} else {
			route.distance = RTMP_TUPLE_DISTANCE(cursor) + 1;
		}
		
		rt_touch(global_routing_table, route);
	}
}

void app_rtmp_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) == 1) {
		handle_rtmp_update_packet(packet);
	}

	freebuf(packet);
}

void app_rtmp_idle(void* dummy) {
	while (1) {
		// Every 20s...
		vTaskDelay(20000 / portTICK_PERIOD_MS);
		
		// Send out stats (do it before aging so that good routes show as good)
		char* new_stats = rt_stats(global_routing_table);
		char* old_stats = atomic_exchange(&stats_routing_table, new_stats);
		if (old_stats != NULL) {
			free(old_stats);
		}
		
		// And age out older routes
		rt_prune(global_routing_table);
		
	}
}

void app_rtmp_start(void) {
	global_routing_table = rt_new();
}
