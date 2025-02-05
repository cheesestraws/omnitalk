#pragma once

#include <stdint.h>

struct snap_hdr {
	uint8_t dest_sap;
	uint8_t src_sap;
	uint8_t control_byte;
	
	// Whoever made SNAP protocol discriminators 5 bytes long has made me
	// grumpy.
	uint8_t proto_discriminator_top_byte;
	uint32_t proto_discriminator_bottom_bytes;
} __attribute__((packed));

typedef struct snap_hdr snap_hdr_t;