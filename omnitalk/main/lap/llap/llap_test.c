#include "lap/llap/llap_test.h"

#include "mem/buffers.h"
#include "mem/buffers_test.h"
#include "proto/ddp.h"
#include "test.h"

bool llap_extract_ddp_packet(buffer_t *buf);
TEST_FUNCTION(test_llap_extract_ddp_packet) {
	char* packet;
	size_t packet_len;
	buffer_t *buf;
	bool result;
	
	// First, a packet with short headers, delivered over LLAP.  This is an RTMP 
	// broadcast packet.
	packet="\xff\x01\x01\x00\x18\x01\x01\x01\x00\x0c\x08\x01\x00\x00\x82\x00\x03\x80\x00\x0a\x82\x00\x0b\x00\x00\x0c\x00";
	packet_len = 27;
	buf = buf_from_string(packet, 3, packet_len);
	result = llap_extract_ddp_packet(buf);
	
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
	result = llap_extract_ddp_packet(buf);

	TEST_ASSERT(result);
	TEST_ASSERT(DDP_DSTSOCK(buf) == 4);
	TEST_ASSERT(DDP_DST(buf) == 135);
	TEST_ASSERT(DDP_DSTNET(buf) == 12);
	TEST_ASSERT(DDP_TYPE(buf) == 4);
	freebuf(buf);
	
	// Too short
	packet="\x87\x01";
	packet_len = 2;
	buf = buf_from_string(packet, 0, packet_len);
	result = llap_extract_ddp_packet(buf);
	TEST_ASSERT(!result);
	freebuf(buf);


	// Bad LLAP type
	packet="\xff\x01\x04\x00\x18\x01\x01\x01\x00\x0c\x08\x01\x00\x00\x82\x00\x03\x80\x00\x0a\x82\x00\x0b\x00\x00\x0c\x00";
	packet_len = 27;
	packet_len = 4;
	buf = buf_from_string(packet, 3, packet_len);
	result = llap_extract_ddp_packet(buf);
	TEST_ASSERT(!result);
	freebuf(buf);
	
	TEST_OK();
}
