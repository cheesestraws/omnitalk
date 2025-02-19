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
	lap_void_handler acquire_address;
	
	QueueHandle_t inbound;
	QueueHandle_t outbound;
};
