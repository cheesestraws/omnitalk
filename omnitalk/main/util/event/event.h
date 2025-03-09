#pragma once

#include <stdbool.h>

#define EVENT_MAX_CALLBACKS 5

typedef void(*event_callback_t)(void* param);

typedef struct {
	int callback_count;
	event_callback_t callbacks[EVENT_MAX_CALLBACKS];
} event_t;

bool event_add_callback(event_t* event, event_callback_t callback);
void event_fire(event_t* event, void* param);
