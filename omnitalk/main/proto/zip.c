#include "proto/zip.h"

static zip_zone_tuple_t* zip_reply_tuple_at(buffer_t *packet, uint8_t *ptr_within_packet) {
	if (!packet->ddp_ready) {
		return NULL;
	}
		
	size_t offset = ptr_within_packet - packet->ddp_data;	
	int remaining = packet->ddp_length - offset;
	
	if (remaining < 3) { // 2 bytes network number, 1 byte string length
		return NULL;
	}
	
	uint8_t zone_name_length = ptr_within_packet[2];
	
	if (remaining < (3 + zone_name_length)) {
		return NULL;
	}
	
	return (zip_zone_tuple_t*)ptr_within_packet;
}

zip_zone_tuple_t* zip_reply_get_first_tuple(buffer_t *packet) {
	uint8_t *tuple_ptr = zip_reply_get_zones(packet);
	return zip_reply_tuple_at(packet, tuple_ptr);
}

zip_zone_tuple_t* zip_reply_get_next_tuple(buffer_t *packet, zip_zone_tuple_t* prev_tuple) {
	uint8_t *tuple_ptr = (uint8_t*)prev_tuple + 3 + prev_tuple->zone_name.length;
	return zip_reply_tuple_at(packet, tuple_ptr);
}
