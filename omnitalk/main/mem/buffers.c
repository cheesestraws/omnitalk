#include "mem/buffers.h"

#include <stdbool.h>
#include <stdlib.h>

buffer_t *newbuf(size_t capacity) {
	uint8_t *data = (uint8_t*)calloc(1, capacity);
	buffer_t *buff = (buffer_t*)calloc(1, sizeof(buffer_t));
	
	buff->data = data;
	buff->capacity = capacity;
	
	return buff;
}

void freebuf(buffer_t *buffer) {
	free(buffer->data);
	free(buffer);
}

buffer_t *wrapbuf(void* data, size_t length) {
	buffer_t *buff = calloc(1, sizeof(buffer_t));
	buff->data = (uint8_t*)data;
	buff->capacity = length;
	buff->length = length;
	
	return buff;
}

void resetbuf(buffer_t *buffer) {
	buffer->length = 0;
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