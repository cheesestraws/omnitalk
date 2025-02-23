#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mem/buffers.h"

#define DDP_ADDR_BROADCAST 0xFF

#define DDP_SOCKET_RTMP 1

struct ddp_short_header_s {
	uint8_t dst;
	uint8_t src;
	uint8_t always_one;
	uint16_t datagram_length;
	uint8_t dst_sock;
	uint8_t src_sock;
	uint8_t ddp_type;
	
	uint8_t body[];
} __attribute__((packed));

typedef struct ddp_short_header_s ddp_short_header_t;

struct ddp_long_header_s {
	uint16_t hop_count_and_datagram_length;
	uint16_t ddp_checksum;
	uint16_t dst_network;
	uint16_t src_network;
	uint8_t dst;
	uint8_t src;
	
	uint8_t dst_sock;
	uint8_t src_sock;
	uint8_t ddp_type;
	
	uint8_t body[];
} __attribute__((packed));

typedef struct ddp_long_header_s ddp_long_header_t;

// Some helper macros to extract fields from buffers

#define DDP_DST(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->dst : ((ddp_long_header_t*)((b)->ddp_data))->dst)
#define DDP_SRC(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->src : ((ddp_long_header_t*)((b)->ddp_data))->src)
#define DDP_DSTNET(b) ((b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->dst_network)
#define DDP_SRCNET(b) ((b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->src_network)
#define DDP_DSTSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->dst_sock : ((ddp_long_header_t*)((b)->ddp_data))->dst_sock)
#define DDP_SRCSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->src_sock : ((ddp_long_header_t*)((b)->ddp_data))->src_sock)
#define DDP_TYPE(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->ddp_type : ((ddp_long_header_t*)((b)->ddp_data))->ddp_type)

static inline bool ddp_packet_is_mine(lap_t *lap, buffer_t *packet) {
	if (packet->ddp_type == BUF_SHORT_HEADER && DDP_DST(packet) == lap->my_address) {
	
		return true;   
	}
	
	if (packet->ddp_type == BUF_LONG_HEADER &&
	    DDP_DST(packet) == lap->my_address &&
	    DDP_DSTNET(packet) >= lap->network_range_start &&
	    DDP_DSTNET(packet) <= lap->network_range_end) {
	
		return true;
	}
	
	if (DDP_DSTNET(packet) == 0 && DDP_DST(packet) == DDP_ADDR_BROADCAST) {
		return true;   
	}

	
	if (packet->ddp_type == BUF_LONG_HEADER &&
	    DDP_DST(packet) == DDP_ADDR_BROADCAST &&
	    DDP_DSTNET(packet) >= lap->my_network) {
	
		return true;
	}
	
	// in theory, we can have a DDP long-header packet being sent on a
	// LocalTalk network with a network ID of 0; this is pointless but
	// legit.
	if (packet->ddp_type == BUF_LONG_HEADER &&
	    DDP_DST(packet) == lap->my_address &&
	    DDP_DSTNET(packet) == 0) {
	
		return true;
	}

	return false;
}
