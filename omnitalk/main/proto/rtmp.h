#pragma once

#include <stdint.h>

#include <lwip/inet.h>

#include "mem/buffers.h"
#include "proto/ddp.h"

struct rtmp_response_s {
	uint16_t senders_network;
	uint8_t id_length_bits;
	uint8_t node_id; // cheating: node ids are always 1 byte
} __attribute__((packed));

typedef struct rtmp_response_s rtmp_response_t;

struct rtmp_tuple_s {
	uint16_t range_start;
	uint8_t ext_flag_and_distance;
	uint16_t range_end;
	uint8_t always_0x82;
} __attribute__((packed));

typedef struct rtmp_tuple_s rtmp_tuple_t;

void print_rtmp_tuple(rtmp_tuple_t *tuple);

#define RTMP_TUPLE_IS_EXTENDED(t) (((t)->ext_flag_and_distance & 0x80) == 0x80)
#define RTMP_TUPLE_DISTANCE(t) ((t)->ext_flag_and_distance & 0x7f)
#define RTMP_TUPLE_NETWORK(t) ntohs((t)->range_start)
#define RTMP_TUPLE_RANGE_START(t) ntohs((t)->range_start)
#define RTMP_TUPLE_RANGE_END(t) (RTMP_TUPLE_IS_EXTENDED((t)) ? ntohs((t)->range_end) : RTMP_TUPLE_RANGE_START((t)))

// The "dummy" tuple is the fake tuple on a nonextended network which carries the
// version
#define RTMP_TUPLE_IS_DUMMY(t) ((t)->range_start == 0 && (t)->ext_flag_and_distance == 0x82)


struct rtmp_packet_s {
	uint16_t router_network;
	uint8_t id_len;
	uint8_t router_node_id;
	uint8_t tuples[];
} __attribute__((packed));

typedef struct rtmp_packet_s rtmp_packet_t;

// Macros take a buffer_t* NOT a rtmp_packet_t*
#define RTMP_ROUTER_NETWORK(b) ntohs(((rtmp_packet_t*)(DDP_BODY((b))))->router_network)
#define RTMP_ID_LEN(b) (((rtmp_packet_t*)(DDP_BODY((b))))->id_len)
#define RTMP_ROUTER_NODE_ID(b) (((rtmp_packet_t*)(DDP_BODY((b))))->id_len)
#define RTMP_TUPLES(b) ((rtmp_tuple_t*)&(((rtmp_packet_t*)(DDP_BODY((b))))->tuples[0]))
#define RTMP_TUPLELEN(b) (DDP_BODYLEN((b)) - sizeof(rtmp_packet_t))

static inline rtmp_tuple_t* get_next_rtmp_tuple(buffer_t *packet, rtmp_tuple_t *current_tuple) {
	uint8_t *tupleref = (uint8_t*)current_tuple;
	
	// are we even in the tuple list?
	if (tupleref < (uint8_t*)(RTMP_TUPLES(packet))) {
		return NULL;
	}

	// is our current tuple extended?  Advance cursor appropriately.
	if (RTMP_TUPLE_IS_DUMMY((rtmp_tuple_t*)tupleref)) {
		tupleref += 3;
	} else if (RTMP_TUPLE_IS_EXTENDED((rtmp_tuple_t*)tupleref)) {
		tupleref += sizeof(rtmp_tuple_t);
	} else {
		tupleref += 3;
	}
	
	int offset = tupleref - (uint8_t*)(RTMP_TUPLES(packet));
	int remaining_space = RTMP_TUPLELEN(packet) - offset;
	rtmp_tuple_t *next_tuple = (rtmp_tuple_t*)tupleref;
	
	if (RTMP_TUPLE_IS_EXTENDED(next_tuple) && remaining_space >= sizeof(rtmp_tuple_t)) {
		return next_tuple;
	} else if (!RTMP_TUPLE_IS_EXTENDED(next_tuple) && remaining_space >= 3) {
		return next_tuple;
	}
	
	return NULL;
}

static inline rtmp_tuple_t* get_first_rtmp_tuple(buffer_t *packet) {
	rtmp_tuple_t *tuple = RTMP_TUPLES(packet);

	// Is there room for one tuple?
	if (RTMP_TUPLELEN(packet) < 3) {
		return NULL;
	}
	
	if (RTMP_TUPLE_IS_DUMMY(tuple)) {
		return get_next_rtmp_tuple(packet, tuple);
	}
	
	// A non-extended tuple fits in three bytes, we can
	// return this pointer
	if (!RTMP_TUPLE_IS_EXTENDED(tuple)) {
		return tuple;
	}
	
	// We must have an extended tuple now, do we have room for one?
	if (RTMP_TUPLELEN(packet) < sizeof(rtmp_tuple_t)) {
		return NULL;
	}
	
	return tuple;
}
