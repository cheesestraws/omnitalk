#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lwip/inet.h>

#include "mem/buffers.h"
#include "proto/ddp.h"

struct zip_query_packet_s {
	uint8_t always_one;
	uint8_t network_count;
	uint16_t networks[];
} __attribute__((packed));

static inline void zip_qry_setup_packet(buffer_t* buff, uint8_t network_count) {
	struct zip_query_packet_s *packet = (struct zip_query_packet_s*)(DDP_BODY(buff));
	packet->always_one = 1;
	packet->network_count = network_count;
	buff->ddp_payload_length = sizeof(struct zip_query_packet_s) + (2 * network_count);
}

static inline bool zip_qry_set_network(buffer_t* buff, int idx, uint16_t network) {
	struct zip_query_packet_s *packet = (struct zip_query_packet_s*)(DDP_BODY(buff));

	if (unlikely(idx >= packet->network_count)) {
		return false;
	}
	
	packet->networks[idx] = htons(network);
	return true;
}
