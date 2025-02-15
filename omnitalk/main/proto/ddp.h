#pragma once

#include <stdint.h>

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
