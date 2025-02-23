#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_err.h>

#include "net/transport.h"

typedef struct lap_s lap_t;

typedef esp_err_t(*lap_void_handler)(lap_t*);

// LAPs take in l2 frames/protocols from a transport
// and turn them into DDP frames.
struct lap_s {
	int id;

	void* info; // protocol-specific
	void* stats; // protocol-specific
	char* name;
	transport_t* transport;
	
	lap_void_handler enable;
	lap_void_handler disable;
	
	QueueHandle_t inbound;
	QueueHandle_t outbound;
	
	uint8_t my_address;
	uint16_t my_network;
	uint16_t network_range_start;
	uint16_t network_range_end;
};
