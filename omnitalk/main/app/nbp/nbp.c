#include "app/nbp/nbp.h"

#include <stdio.h>

#include "app/app.h"
#include "mem/buffers.h"
#include "net/common.h"
#include "proto/ddp.h"
#include "proto/nbp.h"
#include "util/pstring.h"
#include "web/stats.h"
#include "ddp_send.h"
#include "global_state.h"

static void send_nbp_response(uint16_t dest_net, uint8_t dest_node, uint8_t dest_socket,
								char* nbp_object, char* nbp_type, uint8_t socket, uint8_t their_id) {
								
	uint16_t my_network;
	uint8_t my_node;
	if (!lap_registry_get_best_address(global_lap_registry, &my_network, &my_node)) {
		return;
	}
													
	buffer_t *buff = newbuf_ddp();
	buf_expand_payload(buff, sizeof(nbp_packet_t) + sizeof(struct nbp_tuple_s));
	((nbp_packet_t*)buff->ddp_payload)->nbp_id = their_id;
	((nbp_packet_t*)buff->ddp_payload)->function_and_tuple_count = (NBP_LKUP_REPLY << 4) | 1;
	
	// Set tuple fields
	struct nbp_tuple_s *tuple = (struct nbp_tuple_s*)NBP_PACKET_TUPLES(buff);
	NBP_TUPLE_SET_NETWORK(tuple, my_network);
	NBP_TUPLE_SET_NODE(tuple, my_node);
	NBP_TUPLE_SET_SOCKET(tuple, socket);
	buffer_append_cstring_as_pstring(buff, nbp_object);
	buffer_append_cstring_as_pstring(buff, nbp_type);
	buffer_append_cstring_as_pstring(buff, global_lap_registry->best_zone_cache);
		
	if (!ddp_send(buff, 2, dest_net, dest_node, dest_socket, 2)) {
		stats.nbp_out_errors__type_reply__err_ddp_send_failed++;
		freebuf(buff);
		return;
	}
	
	stats.nbp_out_packets__function_LkUp_reply++;
}

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
		
	// Iterate through apps, see if we have anything that matches
	for (int i = 0; unicast_apps[i].socket_number != 0; i++) {
		// Does it match the object being searched for?
		if (unicast_apps[i].nbp_object == NULL ||
		    !pstring_matches_cstring_nbp(nbp_tuple_get_object(tuple), unicast_apps[i].nbp_object)) {
			
			continue;		    
		}
		
		// Does it match the type?
		if (unicast_apps[i].nbp_type == NULL ||
		    !pstring_matches_cstring_nbp(nbp_tuple_get_type(tuple), unicast_apps[i].nbp_type)) {
			
			continue;		    
		}

		send_nbp_response(
			// The address to send the response to is the address information in the 
			// query tuple
			NBP_TUPLE_NETWORK(tuple), NBP_TUPLE_NODE(tuple), NBP_TUPLE_SOCKET(tuple),
			
			// The object, type and socket come out of the apps table
			unicast_apps[i].nbp_object, unicast_apps[i].nbp_type, 
			unicast_apps[i].socket_number,
			
			NBP_PACKET_ID(packet)
		);
	}
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
}
