#pragma once
#include "lap_types.h"

#include <stdbool.h>

#include <lwip/prot/ethernet.h>

#include "mem/buffers.h"

bool lsend(lap_t* lap, buffer_t *buff);

// The LAP will keep the reference to zone, do not free it
void lap_set_my_zone(lap_t *lap, pstring* zone);

bool lap_supports_ether_multicast(lap_t *lap);
struct eth_addr lap_ether_multicast_for_zone(lap_t *lap, pstring *zone);
