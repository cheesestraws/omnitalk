#pragma once

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_err.h>

#include "mem/buffers.h"

// transport.{c,h} defines the interface for a transport; a transport
// is something that can ship l2-ish frames out of the router.

// The types are defined in transport_types.h, to break a circular dependency
#include "net/transport_types.h"

esp_err_t enable_transport(transport_t* transport);
esp_err_t disable_transport(transport_t* transport);
esp_err_t set_transport_node_address(transport_t* transport, uint8_t node_address);
bool transport_supports_ether_multicast(transport_t* transport);
struct eth_addr transport_ether_multicast_for_zone(transport_t* transport, pstring* zone);


void wait_for_transport_ready(transport_t* transport);
void mark_transport_ready(transport_t* transport);

// trecv is a utility function to receive a frame from a transport,
// blocking until a frame is available.
buffer_t* trecv(transport_t* transport);

buffer_t* trecv_with_timeout(transport_t* transport, TickType_t timeout);

// tsend is a utility function to send a frame through a transport,
// never blocking at all.  It returns true if the frame was sent and
// ownership of the buffer was transferred to the transport, false
// otherwise
bool tsend(transport_t* transport, buffer_t *buff);

// tsend_and_block is like tsend but blocks indefinitely
bool tsend_and_block(transport_t* transport, buffer_t *buff);
