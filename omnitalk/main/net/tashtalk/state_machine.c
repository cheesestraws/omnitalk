#include <strings.h>
#include <stdbool.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mem/buffers.h"
#include "net/common.h"
#include "net/tashtalk/state_machine.h"
#include "net/tashtalk/uart.h"
#include "proto/llap.h"
#include "util/crc.h"
#include "web/stats.h"

static const char* TAG = "tashtalk";

tashtalk_rx_state_t* new_tashtalk_rx_state(QueueHandle_t output_queue) {
	tashtalk_rx_state_t* ns = calloc(1, sizeof(tashtalk_rx_state_t));
	
	if (ns == NULL) {
		return NULL;
	}
	
	ns->output_queue = output_queue;
	
	return ns;
}

static void append(buffer_t* packet, unsigned char byte) {
	if (packet->length < packet->capacity) {
		packet->data[packet->length] = byte;
		packet->length++;
	} else {
		ESP_LOGE(TAG, "buffer overflow in packet");
		stats.transport_in_errors__transport_localtalk__err_frame_too_long++;
	}
}

static void do_something_sensible_with_packet(tashtalk_rx_state_t* state) {
	stats.transport_in_frames__transport_localtalk++;
	
	if (state->send_output_to_queue) {	
		// Is frame long enough to be valid?
		if (state->packet_in_progress->length > 2) {
			// chop off CRC, now it's a packet
			state->packet_in_progress->length -= 2;
		} else {
			stats.transport_in_errors__transport_localtalk__err_frame_too_short++;
			freebuf(state->packet_in_progress);
			return;
		}
		
		// We do not want to forward RTS or CTS
		if (state->packet_in_progress->length == 3 &&
			(state->packet_in_progress->data[2] == LLAP_TYPE_RTS ||
			 state->packet_in_progress->data[2] == LLAP_TYPE_CTS)) {
			stats.tashtalk_rx_control_packets_not_forwarded++;
			freebuf(state->packet_in_progress);
			return;
		}
		
		BaseType_t err = xQueueSendToBack(state->output_queue, &state->packet_in_progress, 0);
		if (err != pdTRUE) {
			stats.transport_in_errors__transport_localtalk__err_lap_queue_full++;
			freebuf(state->packet_in_progress);
		}
	} else {
		freebuf(state->packet_in_progress);
	}
	state->packet_in_progress = NULL;
	
//	uart_check_for_tx_wedge(state->packet_in_progress);
}

void tashtalk_feed(tashtalk_rx_state_t* state, unsigned char byte) {

	// do we have a buffer?
	if (state->packet_in_progress == NULL) {
		state->packet_in_progress = newbuf(ETHERNET_FRAME_LEN);
		crc_state_init(&state->crc);
	}
	
	// Are we in an escape state?
	if (state->in_escape) {
		switch(byte) {
			case 0xFF:
				// 0x00 0xFF is a literal 0x00 byte
				append(state->packet_in_progress, 0x00);
				crc_state_append(&state->crc, 0x00);
				break;
				
			case 0xFD:
				// 0x00 0xFD is a complete frame
				if (!crc_state_ok(&state->crc)) {
					ESP_LOGE(TAG, "/!\\ CRC fail: %d", state->crc);
					stats.transport_in_errors__transport_localtalk__err_bad_crc++;
				}
				
				do_something_sensible_with_packet(state);
				state->packet_in_progress = NULL;
				break;
				
			case 0xFE:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "framing error of %d bytes", 
					state->packet_in_progress->length);
				stats.transport_in_errors__transport_localtalk__err_framing_error++;
				
				freebuf(state->packet_in_progress);
				state->packet_in_progress = NULL;
				break;

			case 0xFA:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "frame abort of %d bytes", 
					state->packet_in_progress->length);
				stats.transport_in_errors__transport_localtalk__err_frame_abort++;
				
				freebuf(state->packet_in_progress);
				state->packet_in_progress = NULL;
				break;			
		}
		
		state->in_escape = false;
	} else if (byte == 0x00) {
		state->in_escape = true;
	} else {
		append(state->packet_in_progress, byte);
		crc_state_append(&state->crc, byte);
	}
}

void tashtalk_feed_all(tashtalk_rx_state_t* state, unsigned char* buf, int count) {
	for (int i = 0; i < count; i++) {
		tashtalk_feed(state, buf[i]);
	}
}

