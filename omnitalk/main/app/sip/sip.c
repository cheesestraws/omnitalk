#include "app/sip/sip.h"

#include "mem/buffers.h"
#include "proto/atp.h"
#include "proto/ddp.h"
#include "proto/sip.h"
#include "util/require/goto.h"
#include "web/stats.h"
#include "ddp_send.h"

static sip_response_t response = {
	.response_type = SIP_ACK,
	.padding = 0,

	.responder_major_version = 1,
	.responder_minor_version = 1,

	.appletalk_major_version = 1,
	.appletalk_minor_version = 0,

	.rom_version = 0x78,
	.system_type = 32, // gestalt.appl has 31 as 'paula's desk macintosh', why not
	.system_class = 1,
	.hardware_config = SIP_HW_FPU_PRESENT | SIP_HW_MMU_PRESENT,
	.rom85_upper = SIP_ROM85_DEFAULTS | SIP_ROM85_128K_ROM | SIP_ROM85_NO_SOFT_POWER,
	.always_ff = 0xff,
	.responder_level = 1,
	.responder_link = 1
};

void app_sip_handler(buffer_t *packet) {
	buffer_t *reply = NULL;

	// Is this an ATP request?
	REQUIRE(DDP_TYPE(packet) == 3, cleanup);
	REQUIRE(packet->ddp_payload_length >= sizeof(atp_packet_t), cleanup);
	
	atp_packet_t *atp = (atp_packet_t*)(packet->ddp_payload);
	REQUIRE(atp->user_data[3] == SIP_SYSTEMINFO, cleanup);
	
	stats.sip_in_packets__function_SystemInfo++;
	
	// Yes, let's prepare a reply
	reply = newbuf_atp();
	REQUIRE(reply != NULL, cleanup);
	
	// Trim the user bytes off: 
	buf_trim_payload(reply, 4);
	buf_append_all(reply, (uint8_t*)&response, sizeof(response));
	buffer_append_cstring_as_pstring(reply, "Omnitalk " GIT_VERSION);
	buffer_append_cstring_as_pstring(reply, "No Finder");
	buffer_append_cstring_as_pstring(reply, "No LaserWriter");
	
	uint16_t afp_vers = 0;
	buf_append_all(reply, (uint8_t*)&afp_vers, 2);
	buffer_append_cstring_as_pstring(reply, "No AFP");

	
	// Fake our ATP credentials, taking a trick from XNU and A/UX
	atp_packet_set_function(reply, ATP_TRESP);
	atp_packet_set_eom(reply, true);
	uint16_t tid = atp_packet_get_transaction_id(packet);
	atp_packet_set_transaction_id(reply, tid);
		
	if (!ddp_send(reply, 253, DDP_SRCNET(packet), DDP_SRC(packet),
	    DDP_SRCSOCK(packet), 3)) {
		stats.sip_out_errors__function_Ack__err_ddp_send_failed++;
	} else {
		stats.sip_out_packets__function_Ack++;
		reply = NULL; // no longer our problem if ddp_reply returns true
	}

cleanup:
	if (reply != NULL) {
		freebuf(reply);
	}
	freebuf(packet);
}
