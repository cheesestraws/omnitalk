#include "app/zip/zip_internal.h"
#include "app/zip/zip.h"

#include <lwip/prot/ethernet.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "proto/zip.h"
#include "table/zip/table.h"
#include "web/stats.h"
#include "ddp_send.h"
#include "global_state.h"

static bool stash_first_zone_name(void* pvt, pstring* zone) {
	pstring** stash = (pstring**)pvt;
	*stash = pstrclone(zone);
	return false;
}

void app_zip_handle_get_net_info(buffer_t *packet) {
	stats.zip_in_GetNetInfo_requests++;

	// is there enough room for a GetNextInfo?
	if (packet->ddp_payload_length < sizeof(zip_get_net_info_req_t)) {
		stats.zip_in_errors__err_truncated_GetNetInfo_request++;
		return;
	}
	
	// What, even the zone name?
	if (packet->ddp_payload_length < sizeof(zip_get_net_info_req_t) + 
		zip_gni_req_get_zone_name(packet)->length) {
	
		stats.zip_in_errors__err_truncated_GetNetInfo_request++;
		return;
	}
	
	// Wonders may never cease, we have a legit packet.
	
	// Do we have zones for this network?
	lap_t *recv_lap = packet->recv_chain.lap;
	uint16_t net = recv_lap->network_range_start;
	if (!zt_network_is_complete(global_zip_table, net)) {
		stats.zip_in_errors__err_GetNetInfo_incomplete_net++;
		return;
	}
	
	// Let's fill in a reply
	buffer_t *reply = newbuf_ddp();
	buf_expand_payload(reply, sizeof(zip_get_info_resp_t));
	zip_packet_set_function(reply, ZIP_GETNETINFO_REPLY);
	
	// Flags
	bool only_one_zone = (zt_count_zones_for(global_zip_table, net) == 1);
	bool use_broadcast = lap_supports_ether_multicast(recv_lap);
	bool zone_invalid = !zt_zone_is_valid_for(global_zip_table, zip_gni_req_get_zone_name(packet), net);
	zip_gni_resp_set_flags(reply, only_one_zone, use_broadcast, zone_invalid);
	
	// Networks
	buf_append_uint16(reply, recv_lap->network_range_start);
	buf_append_uint16(reply, recv_lap->network_range_end);
	
	// The original zone being requested
	buf_append_pstring(reply, zip_gni_req_get_zone_name(packet));
	
	// Now, are we returning the zone the client requested, or a different one?
	pstring *real_zone = NULL;
	if (!zone_invalid) {
		// copy the zone so we can free it without blowing the whole world up
		real_zone = pstrclone(zip_gni_req_get_zone_name(packet));
	} else {
		// "iterate" once to get the real zone
		zt_iterate_zone_names_for_net(global_zip_table, &real_zone, 0,
			recv_lap->network_range_start, stash_first_zone_name);
	}
	
	if (real_zone == NULL) {
		// Something's gone badly awry, bail out
		stats.zip_out_errors__err_no_good_default_zone_for_network++;
		free(reply);
		return;
	}
	
	// Do we have a multicast MAC address?
	if (use_broadcast) {
		// No, just say 0
		buf_append(reply, 0);
	} else {
		struct eth_addr mcaddr = lap_ether_multicast_for_zone(recv_lap, real_zone);
		buf_append(reply, 6);
		buf_append_all(reply, &mcaddr.addr[0], 6);
	}
	
	buf_append_pstring(reply, real_zone);
	
	// That's the whole packet.  Where do we send it?
	// We have two options.  IA specifies that if the source address of the packet
	// is in the receiving port's network range, we unicast; otherwise, we broadcast.
	uint16_t dest_net = DDP_DSTNET(packet);
	uint16_t dest_node = DDP_DST(packet);
	uint16_t dest_socket = DDP_DSTSOCK(packet);
	if (dest_net < recv_lap->network_range_start || dest_net > recv_lap->network_range_end) {
		dest_net = 0;
		dest_node = 0xfF;
	}
	
	// Since it might be a broadcast, we always send it deliberately via the LAP that
	// the packet came in on.
	if (!ddp_send_via(reply, DDP_SOCKET_ZIP, dest_net, dest_node, dest_socket,
		DDP_TYPE_ZIP, recv_lap)) {
		
		stats.zip_out_errors__err_ddp_send_failed++;
		freebuf(reply);
	}
}
