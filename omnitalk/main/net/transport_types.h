#pragma once

#include <lwip/prot/ethernet.h>

#include "util/pstring.h"

// we split transport.h and put the types in a separate header to break a circular
// dependency between transports and buffers.

typedef struct transport_s transport_t;

typedef esp_err_t(*transport_handler)(transport_t*);
typedef esp_err_t(*transport_node_address_handler)(transport_t*, uint8_t);
typedef struct eth_addr (*transport_zone_ether_multicast_handler)(transport_t*, pstring*);

struct transport_s {
	int quality;

	char* kind;
	void* private_data;
	
	EventGroupHandle_t ready_event;
	
	transport_handler enable;
	transport_handler disable;
	transport_node_address_handler set_node_address;
	
	// get_zone_ether_multicast is NULL if the transport does not support ethernet
	// multicast (e.g., if it's not ethernet)
	transport_zone_ether_multicast_handler get_zone_ether_multicast;
		
	QueueHandle_t inbound;
	QueueHandle_t outbound;
};
