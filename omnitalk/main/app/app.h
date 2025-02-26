#pragma once

#include "mem/buffers.h"

typedef void(*app_packet_handler)(buffer_t*);

typedef struct {
	uint8_t socket_number;
	app_packet_handler handler;
} app_t;

extern app_t unicast_apps[];
