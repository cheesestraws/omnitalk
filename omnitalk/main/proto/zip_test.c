#include "proto/zip_test.h"
#include "proto/zip.h"

#include "mem/buffers.h"
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
