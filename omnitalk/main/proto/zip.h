#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lwip/inet.h>

#include "mem/buffers.h"
#include "proto/ddp.h"
#include "util/pstring.h"

struct zip_query_packet_s {
	uint8_t always_one;
	uint8_t network_count;
	uint16_t networks[];
} __attribute__((packed));

typedef struct zip_query_packet_s zip_query_packet_t;

static inline void zip_qry_setup_packet(buffer_t* buff, uint8_t network_count) {
	zip_query_packet_t *packet = (zip_query_packet_t*)(DDP_BODY(buff));
	packet->always_one = 1;
	packet->network_count = network_count;
	buff->ddp_payload_length = sizeof(zip_query_packet_t) + (2 * network_count);
}

static inline uint8_t zip_packet_get_network_count(buffer_t* buff) {
	// both queries and responses have the network count in the same place
	// so we just pretend it's a query packet for now
	zip_query_packet_t *packet = (zip_query_packet_t*)(DDP_BODY(buff));
	return packet->network_count;
}

static inline bool zip_qry_set_network(buffer_t* buff, int idx, uint16_t network) {
	zip_query_packet_t *packet = (zip_query_packet_t*)(DDP_BODY(buff));

	if (unlikely(idx >= packet->network_count)) {
		return false;
	}
	
	packet->networks[idx] = htons(network);
	return true;
}

static inline uint16_t zip_qry_get_network(buffer_t* buff, int idx) {
	zip_query_packet_t *packet = (zip_query_packet_t*)(DDP_BODY(buff));

	if (unlikely(idx >= packet->network_count)) {
		return 0;
	}
	
	return ntohs(packet->networks[idx]);
}

#define ZIP_REPLY 2
#define ZIP_EXTENDED_REPLY 8

struct zip_reply_packet_s {
	uint8_t reply_type;
	uint8_t network_count;
	uint8_t zones[];
} __attribute__((packed));

typedef struct zip_reply_packet_s zip_reply_packet_t;

static inline uint8_t* zip_reply_get_zones(buffer_t *buff) {
	zip_reply_packet_t *packet = (zip_reply_packet_t*)(DDP_BODY(buff));
	return &packet->zones[0];
}

struct zip_zone_tuple_s {
	uint16_t network;
	pstring zone_name;
} __attribute__((packed));



typedef struct zip_zone_tuple_s zip_zone_tuple_t;

zip_zone_tuple_t* zip_reply_get_first_tuple(buffer_t *packet);
zip_zone_tuple_t* zip_reply_get_next_tuple(buffer_t *packet, zip_zone_tuple_t* prev_tuple);

