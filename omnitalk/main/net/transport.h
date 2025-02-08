#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_err.h>

// transport.{c,h} defines the interface for a transport; a transport
// is something that can ship l2-ish frames out of the router.

typedef struct transport_s transport_t;

typedef esp_err_t(*transport_handler)(transport_t*);

struct transport_s {
	char* kind;
	void* private_data;
	
	transport_handler enable;
	transport_handler disable;
		
	QueueHandle_t inbound;
	QueueHandle_t outbound;
};

esp_err_t enable_transport(transport_t* transport);
esp_err_t disable_transport(transport_t* transport);