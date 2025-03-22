#include "proto/atp_test.h"
#include "proto/atp.h"

#include "mem/buffers.h"
#include "test.h"

TEST_FUNCTION(atp_control_info_fields) {
	buffer_t *buff = newbuf_atp();
	
	atp_packet_t* packet = (atp_packet_t*)(buff->ddp_payload);
	
	// Function code
	packet->control_information = 0b10101010;
	TEST_ASSERT(atp_packet_get_function(buff) == 0b10);
	
	TEST_ASSERT(atp_packet_set_function(buff, ATP_TREL /* (3) */));
	TEST_ASSERT(packet->control_information == 0b11101010);
	
	TEST_ASSERT(atp_packet_set_function(buff, ATP_UNKNOWN /* (0) */));
	TEST_ASSERT(packet->control_information == 0b00101010);
	TEST_ASSERT(atp_packet_get_function(buff) == ATP_UNKNOWN);

	// XO
	packet->control_information = 0b10101010;
	TEST_ASSERT(atp_packet_get_xo(buff));
	
	TEST_ASSERT(atp_packet_set_xo(buff, false));
	TEST_ASSERT(packet->control_information == 0b10001010);
	TEST_ASSERT(!atp_packet_get_xo(buff));
	
	TEST_ASSERT(atp_packet_set_xo(buff, true));
	TEST_ASSERT(packet->control_information == 0b10101010);

	// EOM
	packet->control_information = 0b10101010;
	TEST_ASSERT(!atp_packet_get_eom(buff));
	
	TEST_ASSERT(atp_packet_set_eom(buff, true));
	TEST_ASSERT(packet->control_information == 0b10111010);
	TEST_ASSERT(atp_packet_get_eom(buff));
	
	TEST_ASSERT(atp_packet_set_eom(buff, false));
	TEST_ASSERT(packet->control_information == 0b10101010);

	// STS
	packet->control_information = 0b10101010;
	TEST_ASSERT(atp_packet_get_sts(buff));
	
	TEST_ASSERT(atp_packet_set_sts(buff, false));
	TEST_ASSERT(packet->control_information == 0b10100010);
	TEST_ASSERT(!atp_packet_get_sts(buff));
	
	TEST_ASSERT(atp_packet_set_sts(buff, true));
	TEST_ASSERT(packet->control_information == 0b10101010);
	
	// Timeout
	packet->control_information = 0b10101010;
	TEST_ASSERT(atp_packet_get_timeout_indicator(buff) == ATP_2MIN);
	
	TEST_ASSERT(atp_packet_set_timeout_indicator(buff, ATP_8MIN));
	TEST_ASSERT(packet->control_information == 0b10101100);
	TEST_ASSERT(atp_packet_get_timeout_indicator(buff) == ATP_8MIN);

	TEST_ASSERT(atp_packet_set_timeout_indicator(buff, ATP_1MIN));
	TEST_ASSERT(packet->control_information == 0b10101001);
	TEST_ASSERT(atp_packet_get_timeout_indicator(buff) == ATP_1MIN);
	
	TEST_ASSERT(atp_packet_set_timeout_indicator(buff, ATP_4MIN | 0b10000)); // would set a flag if masking bad
	TEST_ASSERT(packet->control_information == 0b10101011);
	TEST_ASSERT(atp_packet_get_timeout_indicator(buff) == ATP_4MIN);
	
	freebuf(buff);
	
	TEST_OK();
}
