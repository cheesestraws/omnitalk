#include "mem/buffers.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lwip/prot/ethernet.h>

#include "proto/ddp.h"
#include "proto/SNAP.h"


static size_t longest_l2_hdr = sizeof(struct eth_hdr) + sizeof(snap_hdr_t);

buffer_t *newbuf(size_t data_capacity, size_t l2_hdr_len) {
	size_t capacity = data_capacity + (longest_l2_hdr - l2_hdr_len);

	uint8_t *data = (uint8_t*)calloc(1, capacity);
	buffer_t *buff = (buffer_t*)calloc(1, sizeof(buffer_t));
	
	buff->mem_top = data;
	buff->data = data + (longest_l2_hdr - l2_hdr_len);
	buff->mem_capacity = data_capacity;
	buff->capacity = data_capacity;
	
	return buff;
}

void freebuf(buffer_t *buffer) {
	free(buffer->mem_top);
	free(buffer);
}

buffer_t *wrapbuf(void* data, size_t length) {
	buffer_t *buff = calloc(1, sizeof(buffer_t));
	buff->mem_top = (uint8_t*)data;
	buff->data = (uint8_t*)data;
	buff->capacity = length;
	buff->mem_capacity = length;
	buff->length = length;
	
	return buff;
}

void buf_trim_l2_hdr_bytes(buffer_t *buffer, size_t bytes) {
	assert(bytes <= buffer->capacity);
	buffer->data += bytes;
	buffer->length -= bytes;
	buffer->capacity -= bytes;
}

void buf_give_me_extra_l2_hdr_bytes(buffer_t *buffer, size_t bytes) {
	assert(buffer->data - bytes >= buffer->mem_top);
	
	buffer->data -= bytes;
	buffer->length += bytes;
	buffer->capacity += bytes;
	
	bzero(buffer->data, bytes);
}

void buf_set_l2_hdr_size(buffer_t *buffer, size_t bytes) {
	assert(buffer->ddp_ready);
	assert(buffer->ddp_data - bytes >= buffer->mem_top);
	
	buffer->data = buffer->ddp_data - bytes;
	buffer->length = buffer->ddp_length + bytes;
	buffer->capacity = buffer->ddp_capacity + bytes;
	
	bzero(buffer->data, bytes);
}

bool buf_setup_ddp(buffer_t *buf, size_t l2_hdr_len, buffer_ddp_type_t ddp_header_type) {
	// Does the packet have room for the L2 header?
	if (buf->length < l2_hdr_len) {
		printf("too short\n");
		return false;
	}
	
	// Yep - is it a ddp packet?
	// For short headers, it's sizeof(ddp_short_header_t) not
	// sizeof(llap_hdr_t) + sizeof(ddp_short_header_t) because
	// the short ddp header subsumes the llap header entirely.
	if (buf->length > sizeof(ddp_short_header_t) &&
	    ddp_header_type == BUF_SHORT_HEADER) {
	    
		buf->ddp_type = BUF_SHORT_HEADER;
		buf->ddp_length = buf->length;
		buf->ddp_capacity = buf->capacity;
		buf->ddp_data = buf->data;
		buf->ddp_payload = buf->ddp_data + sizeof(ddp_short_header_t);
		buf->ddp_payload_length = buf->ddp_length - sizeof(ddp_short_header_t);
		buf->ddp_payload_capacity = buf->ddp_capacity - sizeof(ddp_short_header_t);
	} else if (buf->length > l2_hdr_len + sizeof(ddp_long_header_t) &&
	           ddp_header_type == BUF_LONG_HEADER) {
		buf->ddp_type = BUF_LONG_HEADER;
		buf->ddp_length = buf->length - l2_hdr_len;
		buf->ddp_capacity = buf->capacity - l2_hdr_len;
		buf->ddp_data = buf->data + l2_hdr_len;
		buf->ddp_payload = buf->ddp_data + sizeof(ddp_long_header_t);
		buf->ddp_payload_length = buf->ddp_length - sizeof(ddp_long_header_t);
		buf->ddp_payload_capacity = buf->ddp_capacity - sizeof(ddp_long_header_t);
	} else {
		return false;
	}
	
	buf->ddp_ready = true;
	return true;
}

void printbuf(buffer_t *buffer) {
	printf("buffer @ %p (data @ %p) length %d capacity %d\n", buffer, 
		buffer->data, buffer->length, buffer->capacity);
	printf("transport-flags %" PRIu32 ".  ", buffer->transport_flags);
	
	if (buffer->recv_chain.transport != NULL || buffer->recv_chain.lap != NULL) {
		printf("chain: ");
		if (buffer->recv_chain.transport != NULL) {
			printf("%s", buffer->recv_chain.transport->kind);
		} else {
			printf("(NULL)");
		}
		printf(" -> ");
		if (buffer->recv_chain.lap != NULL) {
			printf("%s", buffer->recv_chain.lap->name);
		} else {
			printf("(NULL)");
		}
		printf(".  ");
	}
	
	if (buffer->ddp_ready) {
		printf("ddp ready\n");
		printf("ddp @ %p length %d capacity %d\n", buffer->ddp_data,
			buffer->ddp_length, buffer->ddp_capacity);
	} else {
		printf("ddp not ready.\n");
	}
	for (int i = 0; i < buffer->length; i++) {
		printf("%02x ", (int)buffer->data[i]);
	}
	printf("\n");
}

void printbuf_as_c_literal(buffer_t *buffer) {
	printf("packet=\"");
	for (int i = 0; i < buffer->length; i++) {
		printf("\\x%02x", (int)buffer->data[i]);
	}
	printf("\";\n");
	printf("packet_len=%d;\n", (int)buffer->length);
}

