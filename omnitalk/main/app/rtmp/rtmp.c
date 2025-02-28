#include "app/rtmp/rtmp.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "proto/rtmp.h"
#include "table/routing/route.h"
#include "table/routing/table.h"

#include "web/stats.h"

static const char* TAG = "RTMP";

static rt_routing_table_t *temporary_routing_table;

static void handle_rtmp_update_packet(buffer_t *packet) {
	if (temporary_routing_table == NULL) {
		temporary_routing_table = rt_new();
	}

	stats.rtmp_update_packets++;
	ESP_LOGI(TAG, "got rtmp update");
	
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
	
	ESP_LOGI(TAG, "update from %d.%d, %d bytes of tuples", (int)(RTMP_ROUTER_NETWORK(packet)),
		(int)(RTMP_ROUTER_NODE_ID(packet)), (int)(RTMP_TUPLELEN(packet)));
		
	rtmp_tuple_t *cursor = get_first_rtmp_tuple(packet);
	
	while (cursor != NULL) {
		rt_route_t route = {
			.range_start = RTMP_TUPLE_RANGE_START(cursor),
			.range_end = RTMP_TUPLE_RANGE_END(cursor),
	
			.outbound_lap = packet->recv_chain.lap,
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
		
		rt_touch(temporary_routing_table, route);
	
		print_rtmp_tuple(cursor);
		cursor = get_next_rtmp_tuple(packet, cursor);
	}
	
	rt_print(temporary_routing_table);
}

void app_rtmp_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) == 1) {
		handle_rtmp_update_packet(packet);
	}

	freebuf(packet);
}

void app_rtmp_idle(void* dummy) {
	ESP_LOGI(TAG, "started rtmp idle task");
	vTaskDelay(portMAX_DELAY);
}

void app_rtmp_start(void) {
	ESP_LOGI(TAG, "started rtmp");
}
