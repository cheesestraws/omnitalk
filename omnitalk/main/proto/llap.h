#pragma once

#include <stdint.h>

#define LLAP_TYPE_ENQ 0x81
#define LLAP_TYPE_ACK 0x82
#define LLAP_TYPE_RTS 0x84
#define LLAP_TYPE_CTS 0x85

struct llap_hdr_s {
	uint8_t dst;
	uint8_t src;
	uint8_t llap_type;
} __attribute__((packed));

typedef struct llap_hdr_s llap_hdr_t;