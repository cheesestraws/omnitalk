#include "mem/buffers.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <lwip/prot/ethernet.h>

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

bool buf_set_ddp_info(buffer_t *buffer, uint32_t ddp_offset, buffer_ddp_type_t ddp_type) {
	if (buffer == NULL || buffer->data == NULL) {
		return false;
	}
	
	if (ddp_offset > buffer->length) {
		return false;
	}
	
	buffer->ddp_type = ddp_type;
	buffer->ddp_length = buffer->length - ddp_offset;
	buffer->ddp_capacity = buffer->capacity - ddp_offset;
	buffer->ddp_data = buffer->data + ddp_offset;
	buffer->ddp_ready = true;
	
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
