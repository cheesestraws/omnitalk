#include "atp.h"

#include "mem/buffers.h"

buffer_t *newbuf_atp() {
	buffer_t *buff = newbuf_ddp();
	if (buff != NULL) {
		buf_expand_payload(buff, sizeof(atp_packet_t));
	}
	return buff;
}
