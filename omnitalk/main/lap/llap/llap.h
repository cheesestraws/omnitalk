#pragma once

#include <stdatomic.h>

#include "lap/lap.h"
#include "lap/registry.h"
#include "runloop_types.h"

#define LLAP_OUTBOUND_QUEUE_SIZE 60

typedef enum {
	LLAP_ACQUIRING_ADDRESS = 0,
	LLAP_ACQUIRING_NETINFO,
	LLAP_RUNNING
} llap_interface_state_t;

typedef struct {
	_Atomic llap_interface_state_t state;
	
	_Atomic uint8_t node_addr;
	_Atomic uint16_t discovered_net;
	_Atomic uint16_t discovered_seeding_node;
} llap_info_t;

lap_t *start_llap(char* name, transport_t *transport, lap_registry_t *registry, runloop_info_t *controlplane, runloop_info_t *dataplane);
