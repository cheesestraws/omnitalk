#include "app/rtmp/rtmp.h"

#include <esp_log.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "proto/rtmp.h"
#include "web/stats.h"

static const char* TAG = "RTMP";

static void handle_rtmp_update_packet(buffer_t *packet) {
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
		print_rtmp_tuple(cursor);
		cursor = get_next_rtmp_tuple(packet, cursor);
	}
}

void app_rtmp_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) == 1) {
		handle_rtmp_update_packet(packet);
	}

	freebuf(packet);
}
