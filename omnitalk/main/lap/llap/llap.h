#pragma once

#include <stdatomic.h>

#include "lap/lap.h"

typedef enum {
	LLAP_ACQUIRING_ADDRESS = 0,
	LLAP_ACQUIRING_NETINFO,
	LLAP_RUNNING
} llap_interface_state_t;

typedef struct {
	_Atomic llap_interface_state_t state;
	
	_Atomic uint8_t node_addr;
} llap_info_t;

lap_t *start_llap(char* name, transport_t *transport);