#pragma once

#include "lap/lap.h"
#include "net/transport_types.h"

// A chain is the list of things a packet went through either on its way in
// or its way out.
typedef struct {
	transport_t* transport;
	lap_t* lap;
} net_chain_t;
