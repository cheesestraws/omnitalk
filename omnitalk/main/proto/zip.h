#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lwip/inet.h>

#include "mem/buffers.h"
#include "proto/ddp.h"
#include "util/pstring.h"

#define ZIP_QUERY 1
#define ZIP_REPLY 2
#define ZIP_EXTENDED_REPLY 8

struct zip_packet_s {
	uint8_t function;
	uint8_t network_count;
	uint8_t payload[];
} __attribute__((packed));

typedef struct zip_packet_s zip_packet_t;

#define ZIP_FUNCTION(X) (((zip_packet_t*)(DDP_BODY((X))))->function)

static inline uint8_t zip_packet_get_network_count(buffer_t* buff) {
	zip_packet_t *packet = (zip_packet_t*)(DDP_BODY(buff));
	return packet->network_count;
}

static inline bool zip_qry_set_network(buffer_t* buff, int idx, uint16_t network) {
	zip_packet_t *packet = (zip_packet_t*)(DDP_BODY(buff));

	if (unlikely(idx >= packet->network_count)) {
		return false;
	}
	
	((uint16_t*)packet->payload)[idx] = htons(network);
	return true;
}

static inline uint16_t zip_qry_get_network(buffer_t* buff, int idx) {
	zip_packet_t *packet = (zip_packet_t*)(DDP_BODY(buff));

	if (unlikely(idx >= packet->network_count)) {
		return 0;
	}
	
	return ntohs(((uint16_t*)packet->payload)[idx]);
}

static inline uint8_t* zip_reply_get_zones(buffer_t *buff) {
	zip_packet_t *packet = (zip_packet_t*)(DDP_BODY(buff));
	return &packet->payload[0];
}


static inline void zip_qry_setup_packet(buffer_t* buff, uint8_t network_count) {
	zip_packet_t *packet = (zip_packet_t*)(DDP_BODY(buff));
	packet->function = ZIP_QUERY;
	packet->network_count = network_count;
	buff->ddp_payload_length = sizeof(zip_packet_t) + (2 * network_count);
}


struct zip_zone_tuple_s {
	uint16_t network;
	pstring zone_name;
} __attribute__((packed));

#define ZIP_TUPLE_NETWORK(X) (ntohs((X)->network))
#define ZIP_TUPLE_ZONE_NAME(X) ((X)->zone_name)

typedef struct zip_zone_tuple_s zip_zone_tuple_t;

zip_zone_tuple_t* zip_reply_get_first_tuple(buffer_t *packet);
zip_zone_tuple_t* zip_reply_get_next_tuple(buffer_t *packet, zip_zone_tuple_t* prev_tuple);

