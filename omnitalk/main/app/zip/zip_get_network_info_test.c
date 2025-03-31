#include "app/zip/zip_get_network_info_test.h"
#include "app/zip/zip_internal.h"
#include "app/zip/zip.h"

#include <stdbool.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/zip.h"
#include "table/zip/table.h"
#include "global_state.h"

static bool reply_was_sent;
static buffer_t *reply_buffer;
static pstring* got_multicast_request_for;

static struct eth_addr dummy_multicast_handler(transport_t* t, pstring* p) {
	got_multicast_request_for = p;
	return (struct eth_addr){ .addr = { 1, 2, 3, 4, 5, 6 } };
}

static bool lsend_record(lap_t* lap, buffer_t* buffer) {
	reply_was_sent = true;
	reply_buffer = buffer;
	return true;
}

TEST_FUNCTION(test_zip_get_net_info) {
	buffer_t *buff;

	// Let's create a global zip table
	global_zip_table = zt_new();
	// Create a couple of networks with some zones
	zt_add_net_range(global_zip_table, 1, 10);
	zt_add_net_range(global_zip_table, 11, 20);
	
	// Network 1 has no zones yet and is not yet complete
	zt_add_zone_for(global_zip_table, 11, (pstring*)"\x05Zone1");
	zt_mark_network_complete(global_zip_table, 11);
		
	// A pretend interface for the "packet" to come through
	transport_t dummy_transport = { 0 };
	lap_t dummy_lap = {
		.transport = &dummy_transport,
		.my_address = 1,
		.my_network = 1,
		.network_range_start = 1,
		.network_range_end = 10
	};
	
	// A fake GNI packet
	buff = newbuf_ddp();
	buf_expand_payload(buff, sizeof(zip_get_net_info_req_t));
	buff->ddp_payload[0] = ZIP_GETNETINFO;
	buff->recv_chain.lap = &dummy_lap;
	
	// If we try to do a GNI for an unready network, there should be no reply
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	
	TEST_ASSERT(!reply_was_sent);
	
	// But if for a ready network, we should be good
	dummy_lap.my_network = 11;
	dummy_lap.network_range_start = 11;
	dummy_lap.network_range_end = 20;
	
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	TEST_ASSERT(reply_was_sent);
	
	// Let's examine the reply
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 6); // GetNetInfo reply
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 0xE0); // All three flags set
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[2] == htons(11)); // net start
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[4] == htons(20)); // net end
	TEST_ASSERT(reply_buffer->ddp_payload[6] == 0); // zone name length
	TEST_ASSERT(reply_buffer->ddp_payload[7] == 0); // multicast addr length
	TEST_ASSERT(pstring_eq_cstring((pstring*)(&reply_buffer->ddp_payload[8]), "Zone1"));
	freebuf(reply_buffer);
	
	// If we add a zone name to the request packet, we should get the same zone name back
	// even if it's wrong.
	buf_trim_payload(buff, 1);
	buffer_append_cstring_as_pstring(buff, "ZoneenoZ");
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	TEST_ASSERT(reply_was_sent);
	
	// Examine the reply
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 6); // GetNetInfo reply
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 0xE0); // All three flags set
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[2] == htons(11)); // net start
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[4] == htons(20)); // net end
	TEST_ASSERT(pstring_eq_cstring((pstring*)&reply_buffer->ddp_payload[6], "ZoneenoZ"));
	TEST_ASSERT(reply_buffer->ddp_payload[15] == 0); // multicast addr length
	TEST_ASSERT(pstring_eq_cstring((pstring*)(&reply_buffer->ddp_payload[16]), "Zone1"));
	freebuf(reply_buffer);
	
	// If on the other hand the zone is valid, we should set the flag to tell the
	// client its zone is OK
	buf_trim_payload(buff, 9);
	buffer_append_cstring_as_pstring(buff, "Zone1");
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	TEST_ASSERT(reply_was_sent);
	
	// Examine the reply
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 6); // GetNetInfo reply
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 0x60); // no zone invalid flag
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[2] == htons(11)); // net start
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[4] == htons(20)); // net end
	TEST_ASSERT(pstring_eq_cstring((pstring*)&reply_buffer->ddp_payload[6], "Zone1"));
	TEST_ASSERT(reply_buffer->ddp_payload[12] == 0); // multicast addr length
	TEST_ASSERT(reply_buffer->ddp_payload_length == 13); // no zone name in packet
	freebuf(reply_buffer);

	// If the same thing is done on a transport that supports 802.3 multicast, we should
	// get a multicast address in the packet for the zone.
	dummy_transport.get_zone_ether_multicast = &dummy_multicast_handler;
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	TEST_ASSERT(reply_was_sent);
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 6); // GetNetInfo reply
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 0x20); // only "only one zone" set
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[2] == htons(11)); // net start
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[4] == htons(20)); // net end
	TEST_ASSERT(pstring_eq_cstring((pstring*)&reply_buffer->ddp_payload[6], "Zone1"));
	TEST_ASSERT(reply_buffer->ddp_payload[12] == 6); // multicast addr length
	for (int i = 0; i < 6; i++) {
		TEST_ASSERT(reply_buffer->ddp_payload[13+i] == 1+i); // multicast address
	}
	TEST_ASSERT(pstring_eq_pstring(got_multicast_request_for, (pstring*)"\x05Zone1"));
	dummy_transport.get_zone_ether_multicast = NULL;
	freebuf(reply_buffer);

	
	// If we add an extra zone to the network, however, we should stop setting the 
	zt_add_zone_for(global_zip_table, 11, (pstring*)"\x05Zone2");
	reply_was_sent = false;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_get_net_info(buff);
	TEST_ASSERT(reply_was_sent);
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 6); // GetNetInfo reply
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 0x40); // only "use broadcast" set
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[2] == htons(11)); // net start
	TEST_ASSERT(*(uint16_t*)&reply_buffer->ddp_payload[4] == htons(20)); // net end
	freebuf(reply_buffer);
	
	lap_lsend_mock = NULL;
	TEST_OK();
}
