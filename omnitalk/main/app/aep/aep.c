#include "app/aep/aep.h"

#include <esp_log.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"

static const char* TAG = "AEP";

void app_aep_handler(buffer_t *packet) {
	// AEP packets are of DDP type 4
	if (DDP_TYPE(packet) != 4) {
		goto cleanup;
	}
	
	if (DDP_BODYLEN(packet) < 1) {
		ESP_LOGI(TAG, "AEP packet too short");
		goto cleanup;
	}
	
	if (DDP_BODY(packet)[0] == 2) {
		ESP_LOGI(TAG, "got stray AEP reply, ignoring");
		goto cleanup;
	}
	
	if (DDP_BODY(packet)[0] != 1) {
		ESP_LOGI(TAG, "got unknown AEP function, ignoring");
		goto cleanup;
	}
	
	// Swap the src and dst addresses
	
	uint8_t srcaddr = DDP_SRC(packet);
	uint16_t srcnet = DDP_SRCNET(packet);
	uint8_t srcsock = DDP_SRCSOCK(packet);

	uint8_t dstaddr = DDP_DST(packet);
	uint16_t dstnet = DDP_DSTNET(packet);
	uint8_t dstsock = DDP_DSTSOCK(packet);
	
	ddp_set_dst(packet, srcaddr);
	ddp_set_src(packet, dstaddr);

	if (packet->ddp_type == BUF_LONG_HEADER) {
		ddp_set_dstnet(packet, srcnet);
		ddp_set_srcnet(packet, dstnet);
	}
	
	ddp_set_dstsock(packet, srcsock);
	ddp_set_srcsock(packet, dstsock);
	
	// mark it as a reply
	DDP_BODY(packet)[0] = 2;
	
	
	if (packet->recv_chain.lap == NULL) {
		ESP_LOGE(TAG, "don't know where this packet came from, ignoring it");
		goto cleanup;
	}
	
	if (!lsend(packet->recv_chain.lap, packet)) {
		ESP_LOGE(TAG, "no send");
		goto cleanup;
	}
	
	return;
cleanup:
	freebuf(packet);
}
