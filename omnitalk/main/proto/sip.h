#pragma once

#include <stdint.h>

// SIP - System Information protocol, used by the Responder and InterPoll and the like.
// Details of this were reverse-engineered mostly by robin-fo and bit by me.  See:
// https://68kmla.org/bb/index.php?threads/whats-the-protocol-behind-the-responder.43198/
// Do not consider this file as canonical.

// This structure starts at atp_packet_get_payload() - 4, since it includes
// the four user bytes in the ATP response header.
struct sip_response_s {
	uint8_t response_type;
	uint8_t padding;
	
	uint8_t responder_major_version;
	uint8_t responder_minor_version;

	uint8_t appletalk_major_version;
	uint8_t appletalk_minor_version;
	
	uint8_t rom_version;
	uint8_t system_type; // gestalt (+1 if > responder 1.1)
	uint8_t system_class;
	uint8_t hardware_config; // Bitmask of SIP_HW constants below
	uint8_t rom85_upper; // not actually a version despite adsp.h; see ROM85 constants below
	uint8_t always_ff;
	uint8_t responder_level; // seems to be the same as major responder version
	uint8_t responder_link; // ???
	
	uint8_t strings[];
} __attribute__((packed));

#define SIP_ACK        0x01
#define SIP_NACK       0xff
#define SIP_SYSTEMINFO 0x01
#define SIP_DATALINK   0x06

typedef struct sip_response_s sip_response_t;

#define SIP_HW_HAS_SCSI_PORT      (1<<7)
#define SIP_HW_HAS_NEW_CLOCK_CHIP (1<<6)
#define SIP_HW_XPRAM_VALID        (1<<5)
#define SIP_HW_FPU_PRESENT        (1<<4)
#define SIP_HW_MMU_PRESENT        (1<<3)
#define SIP_HW_ADB_PRESENT        (1<<2)
#define SIP_HW_RUNNING_AUX        (1<<1)
#define SIP_HW_HAS_POWER_MANAGER  (1<<0)

#define SIP_ROM85_DEFAULTS        (0x3f)
#define SIP_ROM85_64K_ROM         (1<<7)
#define SIP_ROM85_128K_ROM        (0<<7)
#define SIP_ROM85_NO_SOFT_POWER   (1<<6)
#define SIP_ROM85_HAS_SOFT_POWER  (0<<6)
