#pragma once

#include <stdint.h>

#include "mem/buffers.h"
#include "proto/ddp.h"
#include "util/pstring.h"

typedef enum {
	NBP_BRRQ = 1,
	NBP_LKUP = 2,
	NBP_LKUP_REPLY = 3,
	NBP_FWDREQ = 4
} nbp_function_t;

struct nbp_packet_s {
	nbp_function_t function : 4;
	int tuple_count : 4;
	
	uint8_t nbp_id;
	
	uint8_t tuples[];
} __attribute__((packed));

typedef struct nbp_packet_s nbp_packet_t;

#define NBP_PACKET_TUPLES(X) (((nbp_packet_t*)(DDP_BODY((X))))->tuples)

struct nbp_tuple_s {
	uint16_t network;
	uint8_t node;
	uint8_t socket;
	uint8_t enumerator;
	uint8_t pstrings[];
} __attribute__((packed));

struct nbp_tuple_s *nbp_get_first_tuple(buffer_t *buff);
struct nbp_tuple_s *nbp_get_next_tuple(buffer_t *buff, struct nbp_tuple_s *prev);

pstring* nbp_tuple_get_object(struct nbp_tuple_s *tuple);
pstring* nbp_tuple_get_type(struct nbp_tuple_s *tuple);
pstring* nbp_tuple_get_zone(struct nbp_tuple_s *tuple);

