#pragma once

#include <stdint.h>

struct rtmp_response_s {
	uint16_t senders_network;
	uint8_t id_length_bits;
	uint8_t node_id; // cheating: node ids are always 1 byte
} __attribute__((packed));

typedef struct rtmp_response_s rtmp_response_t;