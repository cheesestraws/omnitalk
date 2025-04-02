#include "app/zip/zip_test.h"
#include "app/zip/zip_internal.h"
#include "app/zip/zip.h"

#include <stdbool.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "table/routing/route.h"
#include "table/routing/table.h"
#include "table/zip/table.h"
#include "proto/zip.h"
#include "table/zip/table.h"
#include "global_state.h"

static int replies_sent = 0;
static buffer_t* reply_buffer;
static char* test_name;
static uint32_t networks_seen = 0;

static bool lsend_record(lap_t* lap, buffer_t* buffer) {
	replies_sent++;
	
	reply_buffer = buffer;
	
	return true;
}

static bool lsend_multinetwork_hook(lap_t* lap, buffer_t* reply_buffer) {
	SET_TEST_NAME(test_name);
	replies_sent++;
	
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 8); // we always send extended replies
	TEST_ASSERT(reply_buffer->ddp_payload[1] > 0); // some zones
	
	// we only use 8 bit network numbers in tests; in fact we always use numbers < 32
	uint8_t network = reply_buffer->ddp_payload[3];

	// ... so this works.
	networks_seen |= 1 << network;
	
	uint8_t* cursor = &reply_buffer->ddp_payload[2];
	
	if (network == 11) {
		TEST_ASSERT(reply_buffer->ddp_payload[1] == 3); // Three zones
			
		// Each zone has a network number followed by zone name
		TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
		TEST_ASSERT(*cursor == 11);  cursor++;
		TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone1")); cursor += *cursor + 1;

		TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
		TEST_ASSERT(*cursor == 11);  cursor++;
		TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone2")); cursor += *cursor + 1;
	
		TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
		TEST_ASSERT(*cursor == 11);  cursor++;
		TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone3")); cursor += *cursor + 1;
		
			// check we're at the end of the packet
		TEST_ASSERT(cursor == reply_buffer->ddp_payload + reply_buffer->ddp_payload_length);
	}
	
	if (network == 1) {
		TEST_ASSERT(reply_buffer->ddp_payload[1] == 1); // One zone
		
		TEST_ASSERT(*cursor == 0);  cursor++;  // net 1, big endian
		TEST_ASSERT(*cursor == 1);  cursor++;
		TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "ZoneA")); cursor += *cursor + 1;

		// check we're at the end of the packet
		TEST_ASSERT(cursor == reply_buffer->ddp_payload + reply_buffer->ddp_payload_length);
	}
	
	freebuf(reply_buffer);

	return true;
}

TEST_FUNCTION(test_zip_queries) {
	test_name = TEST_NAME;
	
	buffer_t *buff;
	
	// A pretend interface for the "packet" to come through
	transport_t dummy_transport = { 0 };
	lap_t dummy_lap = {
		.transport = &dummy_transport,
		.my_address = 1,
		.my_network = 1,
		.network_range_start = 1,
		.network_range_end = 10
	};
	
	global_routing_table = rt_new();
	rt_route_t catchall = {
		.range_start = 0,
		.range_end = 256,
		.outbound_lap = &dummy_lap,
		.distance = 0
	};
	rt_touch(global_routing_table, catchall);

	// Let's create a global zip table
	global_zip_table = zt_new();
	
	// Create a couple of networks with some zones
	zt_add_net_range(global_zip_table, 1, 10);
	zt_add_net_range(global_zip_table, 11, 20);
	
	// Network 1 has no zones yet and is not yet complete
	
	zt_add_zone_for(global_zip_table, 11, (pstring*)"\x05Zone1");
	zt_add_zone_for(global_zip_table, 11, (pstring*)"\x05Zone2");
	zt_add_zone_for(global_zip_table, 11, (pstring*)"\x05Zone3");
	zt_mark_network_complete(global_zip_table, 11);
	

	// A fake Query packet
	buff = newbuf_ddp();
	ddp_set_src(buff, 1);
	ddp_set_srcnet(buff, 1);
	ddp_set_srcsock(buff, 1);
	buf_expand_payload(buff, sizeof(zip_packet_t));
	buff->ddp_payload[0] = ZIP_QUERY;
	buff->recv_chain.lap = &dummy_lap;
	
	// Let's ask for one network to start with.  Asking for network 1 should result
	// in no reply being sent, as it's not complete.
	buff->ddp_payload[1] = 1; // 1 network
	buf_append_uint16(buff, 1);
	
	replies_sent = 0;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_query(buff);
	
	TEST_ASSERT(replies_sent == 0);
	
	// Asking for network 11 should give us a reply.
	buf_trim_payload(buff, 2);
	buf_append_uint16(buff, 11);
	
	replies_sent = 0;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_query(buff);
	TEST_ASSERT(replies_sent == 1);
		
	// Let's look at the reply we get.
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 8); // we always send extended replies
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 3); // Three zones
	
	// Let's walk the packet and look for stuff
	uint8_t* cursor = &reply_buffer->ddp_payload[2];
	
	// Each zone has a network number followed by 
	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone1")); cursor += *cursor + 1;

	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone2")); cursor += *cursor + 1;
	
	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone3")); cursor += *cursor + 1;
	
	// check we're at the end of the packet
	TEST_ASSERT(cursor == reply_buffer->ddp_payload + reply_buffer->ddp_payload_length);
	
	freebuf(reply_buffer);
	
	// Asking for two networks if one isn't ready should only return the zones for the one
	// that is
	buff->ddp_payload[1] = 2; // 2 networks
	buf_append_uint16(buff, 1);
	replies_sent = 0;
	lap_lsend_mock = &lsend_record;
	app_zip_handle_query(buff);
	TEST_ASSERT(replies_sent == 1);
	
	// Let's look at the reply we get.
	TEST_ASSERT(reply_buffer->ddp_payload[0] == 8); // we always send extended replies
	TEST_ASSERT(reply_buffer->ddp_payload[1] == 3); // Three zones
	
	// Let's walk the packet and look for stuff
	cursor = &reply_buffer->ddp_payload[2];
	
	// Each zone has a network number followed by 
	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone1")); cursor += *cursor + 1;

	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone2")); cursor += *cursor + 1;
	
	TEST_ASSERT(*cursor == 0);  cursor++;  // net 11, big endian
	TEST_ASSERT(*cursor == 11);  cursor++;
	TEST_ASSERT(pstring_eq_cstring((pstring*)cursor, "Zone3")); cursor += *cursor + 1;
	
	// check we're at the end of the packet
	TEST_ASSERT(cursor == reply_buffer->ddp_payload + reply_buffer->ddp_payload_length);
	
	freebuf(reply_buffer);
	
	// Now, if we add a zone to network 1 and mark it complete we should get two packets.
	zt_add_zone_for(global_zip_table, 1, (pstring*)"\x05ZoneA");
	zt_mark_network_complete(global_zip_table, 1);
	
	replies_sent = 0;
	lap_lsend_mock = &lsend_multinetwork_hook;
	app_zip_handle_query(buff);
	TEST_ASSERT(replies_sent == 2);
	TEST_ASSERT(networks_seen == ((1<<1) | (1<<11)));
	
	
	TEST_OK();
}
