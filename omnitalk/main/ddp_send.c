#include "ddp_send.h"

#include <stdio.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "table/routing/route.h"
#include "table/routing/table.h"
#include "web/stats.h"
#include "global_state.h"

bool ddp_send_via(buffer_t *packet, uint8_t src_socket, 
	uint16_t dest_net, uint8_t dest_node, uint8_t dest_socket,
	uint8_t ddp_type, lap_t* via) {

	// Fixup lengths
	buf_set_lengths_from_ddp_payload_length(packet);

	// Fill in header
	ddp_set_datagram_length(packet, packet->ddp_length);
	ddp_clear_checksum(packet); // probably we should set this
	ddp_set_dstnet(packet, dest_net);
	ddp_set_srcnet(packet, via->my_network);
	ddp_set_dst(packet, dest_node);
	ddp_set_src(packet, via->my_address);
	ddp_set_dstsock(packet, dest_socket);
	ddp_set_srcsock(packet, src_socket);
	ddp_set_ddptype(packet, ddp_type);
	
	// Send the thing
	
	printf("ddp_send_via:\n");
	printbuf(packet);
	
	return lsend(via, packet);
	
}

bool ddp_send(buffer_t *packet, uint8_t src_socket, 
	uint16_t dest_net, uint8_t dest_node, uint8_t dest_socket,
	uint8_t ddp_type) {

	rt_route_t route = { 0 };
	bool found;
	
	found = rt_lookup(global_routing_table, dest_net, &route);
	if (!found) {
		stats.ddp_out_errors__err_no_route_for_network++;
		printf("no route to network %d\n", dest_net);
		return false;
	}

	// Set up the nexthop in the send chain for the packet
	packet->send_chain.via_net = route.nexthop.network;
	packet->send_chain.via_node = route.nexthop.node;
	
	return ddp_send_via(packet, src_socket, dest_net, dest_node,
		dest_socket, ddp_type, route.outbound_lap);
}
