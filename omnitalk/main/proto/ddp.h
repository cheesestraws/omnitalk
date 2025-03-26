#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <lwip/inet.h>

#include "mem/buffers.h"

#define DDP_ADDR_BROADCAST 0xFF
#define DDP_MAX_PAYLOAD_LEN 586 // Inside Appletalk 2 ed. p. 4-15, 4-16

#define DDP_SOCKET_RTMP 1
#define DDP_SOCKET_ZIP 6

#define DDP_TYPE_ATP 3
#define DDP_TYPE_ZIP 6

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
#define DDP_DSTNET(b) (ntohs((b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->dst_network))
#define DDP_SRCNET(b) (ntohs((b)->ddp_type == BUF_SHORT_HEADER ? 0 : ((ddp_long_header_t*)((b)->ddp_data))->src_network))
#define DDP_DSTSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->dst_sock : ((ddp_long_header_t*)((b)->ddp_data))->dst_sock)
#define DDP_SRCSOCK(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->src_sock : ((ddp_long_header_t*)((b)->ddp_data))->src_sock)
#define DDP_TYPE(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->ddp_type : ((ddp_long_header_t*)((b)->ddp_data))->ddp_type)
#define DDP_BODY(b) ((b)->ddp_type == BUF_SHORT_HEADER ? ((ddp_short_header_t*)((b)->ddp_data))->body : ((ddp_long_header_t*)((b)->ddp_data))->body)
#define DDP_BODYLEN(b) ((b)->ddp_type == BUF_SHORT_HEADER ? (b)->ddp_length - sizeof(ddp_short_header_t) : (b)->ddp_length - sizeof(ddp_long_header_t))

static inline void ddp_set_dst(buffer_t *buf, uint8_t newdst) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->dst = newdst;
	} else {
		((ddp_long_header_t*)(buf->ddp_data))->dst = newdst;
	}
}

static inline void ddp_set_src(buffer_t *buf, uint8_t newsrc) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->src = newsrc;
	} else {
		((ddp_long_header_t*)(buf->ddp_data))->src = newsrc;
	}
}

static inline void ddp_set_dstnet(buffer_t *buf, uint16_t newdstnet) {
	assert(buf->ddp_type == BUF_LONG_HEADER);
	((ddp_long_header_t*)(buf->ddp_data))->dst_network = htons(newdstnet);
}

static inline void ddp_set_srcnet(buffer_t *buf, uint16_t newsrcnet) {
	assert(buf->ddp_type == BUF_LONG_HEADER);
	((ddp_long_header_t*)(buf->ddp_data))->src_network = htons(newsrcnet);
}

static inline void ddp_set_dstsock(buffer_t *buf, uint8_t newdstsock) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->dst_sock = newdstsock;
	} else {
		((ddp_long_header_t*)(buf->ddp_data))->dst_sock = newdstsock;
	}
}

static inline void ddp_set_srcsock(buffer_t *buf, uint8_t newsrcsock) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->src_sock = newsrcsock;
	} else {
		((ddp_long_header_t*)(buf->ddp_data))->src_sock = newsrcsock;
	}
}

static inline void ddp_set_ddptype(buffer_t *buf, uint8_t newtype) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->ddp_type = newtype;
	} else {
		((ddp_long_header_t*)(buf->ddp_data))->ddp_type = newtype;
	}
}

static inline void ddp_clear_checksum(buffer_t *buf) {
	if (buf->ddp_type == BUF_LONG_HEADER) {
		((ddp_long_header_t*)(buf->ddp_data))->ddp_checksum = 0;
	}
}

static inline void ddp_set_datagram_length(buffer_t *buf, uint16_t length) {
	if (buf->ddp_type == BUF_SHORT_HEADER) {
		((ddp_short_header_t*)(buf->ddp_data))->datagram_length = htons(length & 0x3ff);
	} else {
		// preserve hop count
		uint16_t hop_count_and_length = ntohs(((ddp_long_header_t*)(buf->ddp_data))->hop_count_and_datagram_length);
		uint16_t upper_bits = hop_count_and_length & 0xfc00;
		((ddp_long_header_t*)(buf->ddp_data))->hop_count_and_datagram_length = htons(upper_bits | (length & 0x3ff));
	}
}

static inline bool ddp_append_all(buffer_t *buf, uint8_t *data, size_t count) {
	if (!buf->ddp_ready) {
		return false;
	}
	
	if (buf->ddp_payload_length + count > DDP_MAX_PAYLOAD_LEN) {
		return false;
	}
	return buf_append_all(buf, data, count);
}

static inline bool ddp_append(buffer_t *buf, uint8_t data) {
	return ddp_append_all(buf, &data, 1);
}

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

static inline bool packet_should_be_routed(lap_t *in_lap, buffer_t *packet) {
	return false;
}
