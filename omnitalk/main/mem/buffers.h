#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

	// details about the packet as a whole
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
	size_t ddp_payload_capacity;
	uint8_t* ddp_payload;
	
	// Details about how we got this buffer.  Setting details in here is the
	// responsibility of the receiving LAP.
	net_chain_t recv_chain;
	
	// Details about how we're sending this buffer
	net_chain_t send_chain;
	
} buffer_t;

buffer_t *newbuf(size_t data_capacity, size_t l2_hdr_len);
buffer_t *newbuf_ddp();
void freebuf(buffer_t *buffer_t);
buffer_t *wrapbuf(void* data, size_t length);
bool buf_setup_ddp(buffer_t *buf, size_t l2_hdr_len, buffer_ddp_type_t ddp_header_type);
void printbuf(buffer_t *buffer);
void printbuf_as_c_literal(buffer_t *buffer);

void buf_set_lengths_from_ddp_payload_length(buffer_t *buffer);

void buf_trim_l2_hdr_bytes(buffer_t *buffer, size_t bytes);
void buf_give_me_extra_l2_hdr_bytes(buffer_t *buffer, size_t bytes);
void buf_set_l2_hdr_size(buffer_t *buffer, size_t bytes);

void buf_expand_payload(buffer_t *buffer, size_t bytes);
void buf_trim_payload(buffer_t *buffer, size_t bytes);

static inline bool buf_append_all(buffer_t *buffer, uint8_t *data, size_t bytes) {
	if (buffer == NULL || buffer->data == NULL) {
		return false;
	}
	
	// Do we have room for this?
	if (buffer->length + bytes > buffer->capacity) {
		return false;
	}
	
	// Yes, so work out where to put it
	uint8_t* insert_at = buffer->data + buffer->length;
	memcpy(insert_at, data, bytes);
	
	buffer->length += bytes;
	if (buffer->ddp_ready) {
		buffer->ddp_length += bytes;
		buffer->ddp_payload_length += bytes;
	}
	
	return true;
}

static inline bool buf_append(buffer_t *buffer, uint8_t byte) {
	return buf_append_all(buffer, &byte, 1);
}

static inline bool buffer_append_cstring_as_pstring(buffer_t *buffer, char *str) {
	if (buffer == NULL || buffer->data == NULL) {
		return false;
	}
	
	int len = strlen(str);
	if (len > 255) {
		len = 255;
	}
	
	// Do we have room for this?
	if (buffer->length + len + 1 > buffer->capacity) {
		return false;
	}

	uint8_t* insert_at = buffer->data + buffer->length;
	*insert_at = (uint8_t)len; // length byte first
	insert_at++;
	
	memcpy(insert_at, str, len); // then string
	
	buffer->length += len + 1;
	if (buffer->ddp_ready) {
		buffer->ddp_length += len + 1;
		buffer->ddp_payload_length += len + 1;
	}
	
	return true;
}
