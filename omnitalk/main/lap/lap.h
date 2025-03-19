#pragma once

#include "mem/buffers.h"

#include "lap_types.h"

bool lsend(lap_t* lap, buffer_t *buff);

// The LAP will keep the reference to zone, do not free it
void lap_set_my_zone(lap_t *lap, char* zone);
