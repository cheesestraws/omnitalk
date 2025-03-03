#include "buffers_test.h"
#include "buffers.h"

#include <lwip/prot/ethernet.h>

#include "proto/SNAP.h"
#include "web/stats.h"
#include "test.h"

TEST_FUNCTION(test_newbuf) {
	buffer_t* buf;
	long active_allocs;
	
	// A new buffer must not be null, must have some data space, and mustn't break when
	// freed.  We mustn't leak memory, either.
	active_allocs = stats.mem_all_allocs - stats.mem_all_frees;
	buf = newbuf(1024, 0);
	TEST_ASSERT(buf != NULL);
	TEST_ASSERT(buf->data != NULL);
	freebuf(buf);
	TEST_ASSERT(active_allocs == (stats.mem_all_allocs - stats.mem_all_frees));
	
	// A zero header-length buffer must have enough headroom for an Ethernet and SNAP
	// header.
	buf = newbuf(1024, 0);
	size_t offset = buf->data - buf->mem_top;
	TEST_ASSERT(offset >= (sizeof(struct eth_hdr) + sizeof(snap_hdr_t)));
	freebuf(buf);

	// ... even if we ask for l2 header room
	buf = newbuf(1024, 3);
	offset = buf->data - buf->mem_top;
	TEST_ASSERT(offset >= ((sizeof(struct eth_hdr) + sizeof(snap_hdr_t)) - 3));
	freebuf(buf);

	TEST_OK();
}

TEST_FUNCTION(test_buf_l2hdr_shenanigans) {
	// In order to pull off the thing where we just switch out the layer 2 header in
	// front of a DDP packet without copying the DDP packet, we need to be able to
	// change the length *of* said l2 header, because LocalTalk and Ethernet L2 headers
	// are different lengths (and LToUDP has a "virtual" L2 header that's different
	// again!). 
	//
	// So, we can create a buffer with a 0 length l2 header: this will have enough
	// headroom for the longest l2 header we might need (tested in test_newbuf).
	
	buffer_t* buf;
	size_t max_headroom;
	uint8_t* old_data_ptr;
	
	buf = newbuf(1024, 0);
	max_headroom = buf->data - buf->mem_top;
	old_data_ptr = buf->data;
	
	// Let's ask for some more l2 hdr space and check we've reduced our headroom
	buf_give_me_extra_l2_hdr_bytes(buf, 3);
	buf_give_me_extra_l2_hdr_bytes(buf, 4);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 7));

	// And that our data pointer has indeed moved by 7 bytes
	TEST_ASSERT((buf->data - old_data_ptr) == 7);
	
	// Now reverse course, we've decided we don't want that much space
	buf_trim_l2_hdr_bytes(buf, 2);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 5));
	TEST_ASSERT((buf->data - old_data_ptr) == 5);
	
	// And finally, ask for a specific amount.
	buf_set_l2_hdr_size(buf, 12);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 12));
	TEST_ASSERT((buf->data - old_data_ptr) == 12);

	freebuf(buf);
	
	// We also want to check that any new space for l2 headers that we hand over
	// is zeroed, so that the caller only needs to fill in the l2 headers it needs,
	// and we won't have the ghosts of departed l2 headers (with apologies to Bishop
	// Berkeley) turn up.
	buf = newbuf(1024, 0);
	
	// Fill the headroom with something that isn't zeroes.
	for (uint8_t* cursor = buf->mem_top; cursor < buf->data; cursor++) {
		*cursor = 0xAA;
	}
	
	// Request some more l2 hdr space
	buf_give_me_extra_l2_hdr_bytes(buf, 7);
	for (int i = 0; i < 7; i++) {
		TEST_ASSERT(buf->data[i] == 0);
	}
	
	// And finally request ALL the l2 hdr space.
	buf_set_l2_hdr_size(buf, sizeof(struct eth_hdr) + sizeof(snap_hdr_t));
	for (int i = 0; i < sizeof(struct eth_hdr) + sizeof(snap_hdr_t); i++) {
		TEST_ASSERT(buf->data[i] == 0);
	}
	
	TEST_OK();
}
