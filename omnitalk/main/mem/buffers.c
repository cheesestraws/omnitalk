#include "mem/buffers.h"

#include <stdlib.h>

buffer_t *newbuf(size_t capacity) {
	uint8_t *data = (uint8_t*)malloc(capacity);
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