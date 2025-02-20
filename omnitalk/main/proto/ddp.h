#pragma once

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
#define DDP_DSTNET(b) (b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->dst_network)
#define DDP_SRCNET(b) (b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->src_network)
#define DDP_DSTSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->dst_sock : ((ddp_long_header_t*)((b)->ddp_data))->dst_sock)
#define DDP_SRCSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->src_sock : ((ddp_long_header_t*)((b)->ddp_data))->src_sock)
