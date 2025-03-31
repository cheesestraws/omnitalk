#pragma once
#include "lap_types.h"

#include <stdbool.h>

#include <lwip/prot/ethernet.h>

#include "mem/buffers.h"

// a mock so we can mock out lsend in tests, to test application code.
// Mocks are responsible for disposing of the buffer if they return true.
typedef bool (*lsend_mock_t)(lap_t* lap, buffer_t* buff);
extern lsend_mock_t lap_lsend_mock;

bool lsend(lap_t* lap, buffer_t *buff);

// The LAP will keep the reference to zone, do not free it
void lap_set_my_zone(lap_t *lap, pstring* zone);

bool lap_supports_ether_multicast(lap_t *lap);
struct eth_addr lap_ether_multicast_for_zone(lap_t *lap, pstring *zone);
