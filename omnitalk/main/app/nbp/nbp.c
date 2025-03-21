#include "app/nbp/nbp.h"

#include <stdio.h>

#include "mem/buffers.h"
#include "net/common.h"
#include "proto/ddp.h"
#include "proto/nbp.h"
#include "util/pstring.h"
#include "web/stats.h"
#include "global_state.h"

static char* my_hostname;

static void handle_nbp_lookup(buffer_t *packet) {
	// Does this have a tuple?
	struct nbp_tuple_s *tuple = nbp_get_first_tuple(packet);
	if (tuple == NULL) {
		stats.nbp_in_errors__err_no_tuple++;
	}
	
	// Is this LkUp for us?
	pstring* zone = nbp_tuple_get_zone(tuple);
	if (pstring_eq_cstring(zone, "*")) {
		// A lookup for current zone.  We should only respond if the 
		// "current zone" for that port is our own zone.
		lap_t *packet_lap = packet->recv_chain.lap;
		if (packet_lap == NULL) {
			return;
		}
		if (!strcmp(packet_lap->my_zone, global_lap_registry->best_zone_cache)) {
			return;
		}
	} else if (!pstring_eq_cstring(zone, global_lap_registry->best_zone_cache)) {
		return;
	}
	
	printf("nbp lkup:\n");
	pstring_print(nbp_tuple_get_object(tuple)); printf("\n");
	pstring_print(nbp_tuple_get_type(tuple)); printf("\n");
	pstring_print(nbp_tuple_get_zone(tuple)); printf("\n");
}

void app_nbp_handler(buffer_t *packet) {
	if (DDP_TYPE(packet) != 2) {
		goto cleanup;
	}
	
	switch (nbp_packet_function(packet)) {
	case NBP_INVALID:
		stats.nbp_in_packets__function_invalid++;
		break;
	case NBP_BRRQ:
		stats.nbp_in_packets__function_BrRq++;
		break;
	case NBP_LKUP:
		stats.nbp_in_packets__function_LkUp++;
		handle_nbp_lookup(packet);
		break;
	case NBP_LKUP_REPLY:
		stats.nbp_in_packets__function_LkUp_reply++;
		break;
	case NBP_FWDREQ:
		stats.nbp_in_packets__function_FwdReq++;
		break;
	}
	
cleanup:
	freebuf(packet);
}

void app_nbp_start() {
	my_hostname = generate_hostname();
}
