#include "proto/zip_test.h"
#include "proto/zip.h"

#include <string.h>

#include "mem/buffers.h"
#include "util/pstring.h"
#include "test.h"

TEST_FUNCTION(test_zip_qry_creation) {
	buffer_t *buff;
	
	buff = newbuf_ddp();
	zip_qry_setup_packet(buff, 2);
	TEST_ASSERT(zip_packet_get_network_count(buff) == 2);
	
	// We should be able to set up some network numbers
	TEST_ASSERT(zip_qry_set_network(buff, 0, 8));
	TEST_ASSERT(zip_qry_set_network(buff, 1, 16));
	
	// We can't set off the end of the packet
	TEST_ASSERT(!zip_qry_set_network(buff, 2, 32));
	
	// We should be able to retrieve network numbers, too
	TEST_ASSERT(zip_qry_get_network(buff, 0) == 8);
	TEST_ASSERT(zip_qry_get_network(buff, 1) == 16);
	
	TEST_OK();
}

TEST_FUNCTION(zip_tuple_reading) {
	const char* test_tuples = "\x01\x02\x06""AAAAAA\x02\x03\x05""BBBBB\x03\x04\x04""CCCC\x04\x05\x03";
	const char* test_zones[] = { "AAAAAA", "BBBBB", "CCCC" };

	buffer_t *buff = newbuf_ddp();
	char *tuples = (char*)zip_reply_get_zones(buff);
	strcpy(tuples, test_tuples);
	buff->ddp_payload_length += sizeof(zip_reply_packet_t);
	buff->ddp_payload_length += strlen(test_tuples);
	buf_set_lengths_from_ddp_payload_length(buff);
		
	zip_zone_tuple_t* t;
	int i = 0;
	for (t = zip_reply_get_first_tuple(buff); t != NULL; t = zip_reply_get_next_tuple(buff, t)) {		
		TEST_ASSERT(pstring_eq_cstring(&t->zone_name, test_zones[i]));
		
		i++;
		if (i > 3) {
			TEST_FAIL("too many tuples");
		}
	}
	
	if (i < 3) {
		TEST_FAIL("too few tuples");
	}
	
	TEST_OK();
};
