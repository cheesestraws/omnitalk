#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "net/chain.h"

typedef enum {
	BUF_SHORT_HEADER = 1,
	BUF_LONG_HEADER = 2
} buffer_ddp_type_t;

// If TRANSPORT_FLAG_TASHTALK_CONTROL_FRAME is set on a buffer it will
// be sent to tashtalk without the 0x01 prefix.
#define TRANSPORT_FLAG_TASHTALK_CONTROL_FRAME (1 << 0)

// a buffer is a ... buffer which will hold a DDP packet and some
// L2 framing around it.
typedef struct buffer_s {
	// The memory area the buffer points into
	size_t mem_capacity;
	uint8_t* mem_top;

	// Private flags set by transports
	uint32_t transport_flags;

	// details about the memory buffer as a whole
	size_t length;
	size_t capacity;
	uint8_t* data;
	
	// the l3 packet
	bool ddp_ready;
	buffer_ddp_type_t ddp_type;
	size_t ddp_length;
	size_t ddp_capacity;
	uint8_t* ddp_data;
	size_t ddp_payload_length;
	uint8_t* ddp_payload;
	
	// Details about how we got this buffer.  Setting details in here is the
	// responsibility of the receiving LAP.
	net_chain_t recv_chain;
} buffer_t;

#define BUFFER_HEADER_ROOM(b) ((b)->ddp_data - (b)->data)

buffer_t *newbuf(size_t length, size_t l2_hdr_len);
void freebuf(buffer_t *buffer_t);
buffer_t *wrapbuf(void* data, size_t length);
void printbuf(buffer_t *buffer);

void buf_trim_l2_hdr_bytes(buffer_t *buffer, size_t bytes);
bool buf_set_ddp_info(buffer_t *buffer, uint32_t ddp_offset, buffer_ddp_type_t ddp_type);
