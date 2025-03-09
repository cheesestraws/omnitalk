#include "proto/ddp_test.h"
#include "proto/ddp.h"

#include "mem/buffers.h"
#include "test.h"

TEST_FUNCTION(test_ddp_append) {
	buffer_t *buffer;
	uint8_t *nonsense = malloc(4096);
	
	buffer = newbuf(4099, 3); // allocate a huge buffer
	buffer->length = sizeof(ddp_long_header_t) + 3;
	// and pretend we're doing DDP
	TEST_ASSERT(buf_setup_ddp(buffer, 3, BUF_LONG_HEADER));
	
	// We should be able to append the full amount of data DDP permits
	TEST_ASSERT(ddp_append_all(buffer, nonsense, DDP_MAX_PAYLOAD_LEN));
	TEST_ASSERT(buffer->ddp_payload_length == DDP_MAX_PAYLOAD_LEN);
	TEST_ASSERT(buffer->ddp_length == (DDP_MAX_PAYLOAD_LEN + sizeof(ddp_long_header_t)));
	freebuf(buffer);
	
	// Appending any more should fail, even though there's room in the buffer
	TEST_ASSERT(!ddp_append_all(buffer, nonsense, 1));
	TEST_ASSERT(!ddp_append(buffer, 'a'));
	
	// Reset buffer, let's make sure we can actually write stuff
	buffer->length = sizeof(ddp_long_header_t) + 3;
	TEST_ASSERT(buf_setup_ddp(buffer, 3, BUF_LONG_HEADER));
	TEST_ASSERT(ddp_append(buffer, 'a'));
	TEST_ASSERT(buffer->ddp_payload[0] == 'a');
	
	TEST_OK();
}
