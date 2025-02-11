#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct buffer_s {
	size_t length;
	size_t capacity;
	uint8_t* data;
} buffer_t;

buffer_t *newbuf(size_t length);
void freebuf(buffer_t *buffer_t);
buffer_t *wrapbuf(void* data, size_t length);
void resetbuf(buffer_t *buffer);
