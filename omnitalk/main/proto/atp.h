#pragma once

#include <stdbool.h>

#include <lwip/inet.h>

#include "mem/buffers.h"

typedef enum {
	ATP_UNKNOWN = 0,
	ATP_TREQ = 1,
	ATP_TRESP = 2,
	ATP_TREL = 3
} atp_function_t;

typedef enum {
	ATP_30SEC = 0,
	ATP_1MIN = 1,
	ATP_2MIN = 2,
	ATP_4MIN = 3,
	ATP_8MIN = 4
} atp_timeout_indicator;

struct atp_packet_s {
	uint8_t control_information;
	uint8_t bitmap;
	uint16_t transaction_id;
	uint8_t user_data[4];
	uint8_t payload[];
} __attribute__((packed));

typedef struct atp_packet_s atp_packet_t;

static inline uint8_t* atp_packet_get_payload(buffer_t *buff) {
	if (buff->ddp_payload_length < sizeof(atp_packet_t)) {
		return NULL;
	}
	return ((atp_packet_t*)(buff->ddp_payload))->payload;
}

static inline int atp_packet_get_control_info_field(buffer_t *buff, uint8_t mask, uint8_t shift) {
	if (buff->ddp_payload_length < sizeof(atp_packet_t)) {
		return 0;
	}
	
	uint8_t control_info = ((atp_packet_t*)(buff->ddp_payload))->control_information;
	
	return (control_info >> shift) & mask;
}

static inline bool atp_packet_set_control_info_field(buffer_t *buff, uint8_t mask, uint8_t shift, uint8_t value) {
	if (buff->ddp_payload_length < sizeof(atp_packet_t)) {
		return false;
	}
	
	uint8_t control_info = ((atp_packet_t*)(buff->ddp_payload))->control_information;
	
	control_info &= ~(mask << shift);
	control_info |= ((value & mask) << shift);
	
	((atp_packet_t*)(buff->ddp_payload))->control_information = control_info;
	return true;
}


static inline atp_function_t atp_packet_get_function(buffer_t *buff) {
	return (atp_function_t)atp_packet_get_control_info_field(buff, 3, 6);
}

static inline bool atp_packet_set_function(buffer_t *buff, atp_function_t fn) {
	return atp_packet_set_control_info_field(buff, 3, 6, (uint8_t)fn);
}

static inline bool atp_packet_get_xo(buffer_t *buff) {
	return (bool)atp_packet_get_control_info_field(buff, 1, 5);
}

static inline bool atp_packet_set_xo(buffer_t *buff, bool xo) {
	return (bool)atp_packet_set_control_info_field(buff, 1, 5, (uint8_t)xo);
}

static inline bool atp_packet_get_eom(buffer_t *buff) {
	return (bool)atp_packet_get_control_info_field(buff, 1, 4);
}

static inline bool atp_packet_set_eom(buffer_t *buff, bool eom) {
	return (bool)atp_packet_set_control_info_field(buff, 1, 4, (uint8_t)eom);
}

static inline bool atp_packet_get_sts(buffer_t *buff) {
	return (bool)atp_packet_get_control_info_field(buff, 1, 3);
}

static inline bool atp_packet_set_sts(buffer_t *buff, bool sts) {
	return (bool)atp_packet_set_control_info_field(buff, 1, 3, (uint8_t)sts);
}

static inline atp_timeout_indicator atp_packet_get_timeout_indicator(buffer_t *buff) {
	return (atp_timeout_indicator)atp_packet_get_control_info_field(buff, 7, 0);
}

static inline bool atp_packet_set_timeout_indicator(buffer_t *buff, atp_timeout_indicator value) {
	return (bool)atp_packet_set_control_info_field(buff, 7, 0, (uint8_t)value);
}

static inline uint16_t atp_packet_get_transaction_id(buffer_t *buff) {
	if (buff->ddp_payload_length < sizeof(atp_packet_t)) {
		return 0;
	}
	
	return 	ntohs(((atp_packet_t*)(buff->ddp_payload))->transaction_id);
}

static inline bool atp_packet_set_transaction_id(buffer_t *buff, uint16_t tid) {
	if (buff->ddp_payload_length < sizeof(atp_packet_t)) {
		return false;
	}
	
	((atp_packet_t*)(buff->ddp_payload))->transaction_id = htons(tid);
	return true;
}

buffer_t *newbuf_atp();
