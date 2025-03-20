#include "proto/nbp.h"

#include "mem/buffers.h"

static struct nbp_tuple_s *nbp_tuple_at(buffer_t *buff, uint8_t* ptr_within_packet) {
	uint8_t* cursor = ptr_within_packet;
	if (!buff->ddp_ready) {
		return NULL;
	}

	size_t offset = ptr_within_packet - buff->ddp_data;	
	int remaining = buff->ddp_length - offset;

	// Is there enough remaining for the addresses and so forth?
	if (remaining < sizeof(struct nbp_tuple_s)) {
		return NULL;
	}	
	remaining -= sizeof(struct nbp_tuple_s);
	cursor += sizeof(struct nbp_tuple_s);
	
	// Then room for three strings
	for (int i = 0; i < 3; i++) {
		// Do we have a length?
		if (remaining < 1) {
			return NULL;
		}
		uint8_t string_length = cursor[0];
		
		// Room for the string?
		if (remaining < string_length + 1) {
			return NULL;
		}
		
		remaining -= string_length + 1;
		cursor += string_length + 1;
	}
	
	return (struct nbp_tuple_s*)ptr_within_packet;
}

static size_t nbp_tuple_length(struct nbp_tuple_s* tuple) {
	// Fixed bits
	size_t len = sizeof(struct nbp_tuple_s);
	
	uint8_t *cursor = &(tuple->pstrings[0]);
	
	// and three strings
	for (int i = 0; i < 3; i++) {
		len += *cursor + 1;
		cursor += *cursor;
		cursor += 1;
	}
	
	return len;
}

struct nbp_tuple_s *nbp_get_first_tuple(buffer_t *buff) {
	return nbp_tuple_at(buff, NBP_PACKET_TUPLES(buff));
}

struct nbp_tuple_s *nbp_get_next_tuple(buffer_t *buff, struct nbp_tuple_s *prev) {
	size_t my_len = nbp_tuple_length(prev);
	return nbp_tuple_at(buff, (uint8_t*)prev + my_len);
}

pstring* nbp_tuple_get_object(struct nbp_tuple_s *tuple) {
	return (pstring*)(&(tuple->pstrings[0]));
}

pstring* nbp_tuple_get_type(struct nbp_tuple_s *tuple) {
	uint8_t *cursor = &tuple->pstrings[0];
	cursor += (*cursor + 1);
	return (pstring*)cursor;
}

pstring* nbp_tuple_get_zone(struct nbp_tuple_s *tuple) {
	uint8_t *cursor = &tuple->pstrings[0];
	cursor += (*cursor + 1);
	cursor += (*cursor + 1);
	return (pstring*)cursor;
}

