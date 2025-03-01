#pragma once

#include "lap/lap_types.h"
#include "net/transport_types.h"

// A chain is the list of things a packet went through either on its way in
// or its way out.
typedef struct {
	uint16_t via_net;
	uint8_t via_node;

	transport_t* transport;
	lap_t* lap;
} net_chain_t;
