#include "app/zip/zip_internal.h"

#include <stdbool.h>

#include "mem/buffers.h"
#include "proto/atp.h"
#include "proto/ddp.h"
#include "table/zip/table.h"
#include "util/pstring.h"
#include "web/stats.h"
#include "global_state.h"
#include "ddp_send.h"

const static char* TAG = "ZIP";

int zone_count_in_packet;

static bool add_zone_name_to_buffer(void* bufferp, pstring* zone) {
	buffer_t *buffer = (buffer_t*)bufferp;
	
	if (zone == NULL) {
		// Set LastFlag if we've seen the last zone in the list
		uint8_t *user_data = atp_packet_get_user_data(buffer);
		user_data[0] = 1;
		return true;
	}
	
	if (buffer->ddp_payload_length + zone->length < DDP_MAX_PAYLOAD_LEN) {
		buf_append_pstring(buffer, zone);
		zone_count_in_packet++;
		return true;
	} else {
		return false;
	}
}

void app_zip_handle_get_zone_list(buffer_t *packet) {
	zone_count_in_packet = 0;
	uint8_t *user_data = atp_packet_get_user_data(packet);
	
	// Local or all zones?
	bool only_local_zones;
	if (user_data[0] == 8) {
		only_local_zones = false;
	} else if (user_data[0] == 9) {
		only_local_zones = true;
	} else {
		stats.zip_in_errors__err_atp_dispatch_error++;
		return;
	}
	
	uint16_t start_index;
	start_index = ntohs(*((uint16_t*)&user_data[2]));
	// Zone indexes are 1-based
	start_index--;
	
	/* GetZoneList / GetLocalZones are racy.  If a router joins the network
	   between two GetZoneLists, the meaning of the indexes in the packet will
	   change and the client will get a weird and inconsistent result.  Therefore,
	   we aren't going to bother with doing the whole processing under the lock;
	   it's not like the ZIP reply, where we need to be careful to send everything. */
	   
	uint16_t local_net = 0;
	if (only_local_zones) {
		local_net = DDP_SRCNET(packet);
		
		if (!zt_network_is_complete(global_zip_table, local_net)) {
			// The network is missing or incomplete
			stats.zip_in_errors__err_getzonelist_incomplete_net++;
			return;
		}
	}
	
	buffer_t *buffer = newbuf_atp();
	bool done_something_sensible;
	if (only_local_zones) {
		done_something_sensible = zt_iterate_zone_names_for_net(global_zip_table, buffer, 
			local_net, start_index, add_zone_name_to_buffer);
	} else {
		done_something_sensible = zt_iterate_zone_names(global_zip_table, buffer, 
			start_index, add_zone_name_to_buffer);
	}
	
	if (!done_something_sensible) {
		freebuf(buffer);
		return;
	}
	
	// Fake our ATP credentials, taking a trick from XNU and A/UX
	atp_packet_set_function(buffer, ATP_TRESP);
	atp_packet_set_eom(buffer, true);
	uint16_t tid = atp_packet_get_transaction_id(packet);
	atp_packet_set_transaction_id(buffer, tid);
	
	uint8_t* reply_user_data = atp_packet_get_user_data(buffer);
	*((uint16_t*)&reply_user_data[2]) = htons(zone_count_in_packet);
	
	if (!ddp_send(buffer, 6, DDP_SRCNET(packet), DDP_SRC(packet), DDP_SRCSOCK(packet), DDP_TYPE_ATP)) {
		stats.zip_out_errors__err_ddp_send_failed++;
		freebuf(buffer);
	} else {
		stats.zip_out_replies__kind_getzonelist++;
	}
}
