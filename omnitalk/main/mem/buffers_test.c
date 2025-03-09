#include "buffers_test.h"
#include "buffers.h"

#include <lwip/prot/ethernet.h>

#include "proto/ddp.h"
#include "proto/SNAP.h"
#include "web/stats.h"
#include "test.h"

// buf_from_string generates a buffer from a string (literal) to assist in testing
// with real packet data.
buffer_t* buf_from_string(char* str, size_t l2_hdr_length, size_t frame_length) {
	buffer_t *buf = newbuf(frame_length, l2_hdr_length);
	
	// we are *deliberately* not copying the \0 termination, since it's not part of the
	// packet.
	memcpy(buf->data, str, frame_length);
	buf->length = frame_length;
	
	return buf;
}

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
	// Fake DDP readiness
	buf->ddp_ready = true;
	buf->ddp_data = buf->data;
	
	// Let's ask for some more l2 hdr space and check we've reduced our headroom
	buf_give_me_extra_l2_hdr_bytes(buf, 3);
	buf_give_me_extra_l2_hdr_bytes(buf, 4);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 7));

	// And that our data pointer has indeed moved by 7 bytes
	TEST_ASSERT((old_data_ptr - buf->data) == 7);
	
	// Now reverse course, we've decided we don't want that much space
	buf_trim_l2_hdr_bytes(buf, 2);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 5));
	TEST_ASSERT((old_data_ptr - buf->data) == 5);
	
	// And finally, ask for a specific amount.
	buf_set_l2_hdr_size(buf, 12);
	TEST_ASSERT((buf->data - buf->mem_top) == (max_headroom - 12));
	TEST_ASSERT((old_data_ptr - buf->data) == 12);

	freebuf(buf);
	
	// We also want to check that any new space for l2 headers that we hand over
	// is zeroed, so that the caller only needs to fill in the l2 headers it needs,
	// and we won't have the ghosts of departed l2 headers (with apologies to Bishop
	// Berkeley) turn up.
	buf = newbuf(1024, 0);
	// Fake DDP readiness
	buf->ddp_ready = true;
	buf->ddp_data = buf->data;
	
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

TEST_FUNCTION(test_buf_ddp_setup) {
	char* packet;
	size_t packet_len;
	buffer_t *buf;
	bool result;
	
	// We test with real packet data here.
	
	// First, a packet with short headers, delivered over LLAP.  This is an RTMP 
	// broadcast packet.
	packet="\xff\x01\x01\x00\x18\x01\x01\x01\x00\x0c\x08\x01\x00\x00\x82\x00\x03\x80\x00\x0a\x82\x00\x0b\x00\x00\x0c\x00";
	packet_len = 27;
	buf = buf_from_string(packet, 3, packet_len);
	result = buf_setup_ddp(buf, 3, BUF_SHORT_HEADER);
	
	// Did we manage to extract the DDP packet?
	TEST_ASSERT(result);
	TEST_ASSERT(DDP_DST(buf) == 0xFF);
	TEST_ASSERT(DDP_SRC(buf) == 0x01);
	TEST_ASSERT(DDP_DSTSOCK(buf) == 0x01);
	freebuf(buf);
	
	// Long headers, delivered over LLAP.  This is an AEP packet.
	packet="\x87\x01\x02\x00\x22\xf1\x96\x00\x0c\x00\x08\x87\x1f\x04\x88\x04\x01\x00\x00\x00\x00\x03\xde\xca\x67\x00\x00\x00\x00\xf2\x73\x09\x00\x00\x00\x00\x00";
	packet_len=37;
	buf = buf_from_string(packet, 3, packet_len);
	result = buf_setup_ddp(buf, 3, BUF_LONG_HEADER);

	TEST_ASSERT(result);
	TEST_ASSERT(DDP_DSTSOCK(buf) == 4);
	TEST_ASSERT(DDP_DST(buf) == 135);
	TEST_ASSERT(DDP_DSTNET(buf) == 12);
	TEST_ASSERT(DDP_TYPE(buf) == 4);
	freebuf(buf);
	
	// Long headers, delivered over Ethernet.  This is an NBP packet
	packet="\t\000\a\377\377\377\000\f)\016\0245\000)\252\252\003\b\000\a\200\233\000!4G\000\000\000\004\377s\002\002\002!\001\000\004s\200\000\001=\001=\bEthernet\000\000\000\000\000";
	packet_len = 60;
	buf = buf_from_string(packet, 3, packet_len);
	result = buf_setup_ddp(buf, sizeof(struct eth_hdr) + sizeof(snap_hdr_t) , BUF_LONG_HEADER);
	
	TEST_ASSERT(result);
	TEST_ASSERT(DDP_SRC(buf) == 115);
	TEST_ASSERT(DDP_SRCNET(buf) == 4);
	TEST_ASSERT(DDP_DSTSOCK(buf) == 2);	
	freebuf(buf);
	
	TEST_OK();
}

TEST_FUNCTION(test_buf_append) {
	buffer_t *buffer;
	bool result;
	
	buffer = newbuf(100, 0);
	TEST_ASSERT(buffer->length == 0);
	
	// Append a single byte
	result = buf_append(buffer, 'a');
	TEST_ASSERT(result);
	TEST_ASSERT(buffer->length == 1);
	
	// Reset the length and half-fill the packet with random gibberish
	uint8_t *gibberish = malloc(100);
	buffer->length = 0;
	result = buf_append_all(buffer, gibberish, 50);
	TEST_ASSERT(result);
	TEST_ASSERT(buffer->length == 50);
	
	// Now if we try to add more than 50 more bytes it should fail
	result = buf_append_all(buffer, gibberish, 51);
	TEST_ASSERT(!result);
	TEST_ASSERT(buffer->length == 50);

	// But we can add 50 more, and the buffer will be full
	result = buf_append_all(buffer, gibberish, 100);
	TEST_ASSERT(result);
	TEST_ASSERT(buffer->length == 100);
	
	// And even adding a single further byte should fail.
	result = buf_append(buffer, 'a');
	TEST_ASSERT(!result);
	TEST_ASSERT(buffer->length == 100);

	freebuf(buffer);
	
	// Now let's pretend we're doing DDP
	buffer = newbuf(100, 3);
	// Add a fake l2 header
	TEST_ASSERT(buf_append_all(buffer, gibberish, 3));
	// And a fake L3 header
	TEST_ASSERT(buf_append_all(buffer, gibberish, sizeof(ddp_long_header_t)));
	TEST_ASSERT(buf_setup_ddp(buffer, 3, BUF_LONG_HEADER));
	
	// And validate an assumption to avoid wild goose chases if we break buf_setup_ddp
	TEST_ASSERT(buffer->ddp_length == sizeof(ddp_long_header_t));
	TEST_ASSERT(buffer->ddp_payload_length == 0);
	
	// And append some gubbins.  The DDP details should be updated as well as the l2 ones
	TEST_ASSERT(buf_append_all(buffer, gibberish, 50));
	TEST_ASSERT(buffer->length == 50 + sizeof(ddp_long_header_t) + 3);
	TEST_ASSERT(buffer->ddp_length == 50 + sizeof(ddp_long_header_t));
	TEST_ASSERT(buffer->ddp_payload_length == 50);
	
	// Adding another 50 should fail.
	TEST_ASSERT(!buf_append_all(buffer, gibberish, 50));
	TEST_ASSERT(buffer->length == 50 + sizeof(ddp_long_header_t) + 3);
	TEST_ASSERT(buffer->ddp_length == 50 + sizeof(ddp_long_header_t));
	TEST_ASSERT(buffer->ddp_payload_length == 50);
	
	// But adding 50 - sizeof(ddp_long_header_t) should be fine
	TEST_ASSERT(buf_append_all(buffer, gibberish, 50 - sizeof(ddp_long_header_t)));
	TEST_ASSERT(buffer->length == 103);
	TEST_ASSERT(buffer->ddp_length == 100);
	TEST_ASSERT(buffer->ddp_payload_length == 100 - sizeof(ddp_long_header_t));
	
	freebuf(buffer);
	
	free(gibberish);
	TEST_OK();
}
