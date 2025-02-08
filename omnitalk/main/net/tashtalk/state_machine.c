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
		stats.tashtalk_llap_too_long_count++;
	}
}

static void do_something_sensible_with_packet(tashtalk_rx_state_t* state) {
	stats.tashtalk_llap_rx_frame_count++;
	
	if (state->send_output_to_queue) {
		BaseType_t err = xQueueSendToBack(state->output_queue, &state->packet_in_progress, 0);
		if (err != pdTRUE) {
			stats.tashtalk_inbound_path_queue_full++;
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
					stats.tashtalk_crc_fail_count++;
				}
				
				do_something_sensible_with_packet(state);
				state->packet_in_progress = NULL;
				break;
				
			case 0xFE:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "framing error of %d bytes", 
					state->packet_in_progress->length);
				stats.tashtalk_framing_error_count++;
				
				freebuf(state->packet_in_progress);
				state->packet_in_progress = NULL;
				break;

			case 0xFA:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "frame abort of %d bytes", 
					state->packet_in_progress->length);
				stats.tashtalk_frame_abort_count++;
				
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

/*bool tashtalk_tx_validate(llap_packet* packet) {
	if (packet->length < 5) {
		// Too short
		ESP_LOGE(TAG, "tx packet too short");
		return false;
	}
	
	if (packet->length == 5 && !(packet->packet[2] & 0x80)) {
		// 3 byte packet is not a control packet
		ESP_LOGE(TAG, "tx 3-byte non-control packet, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if ((packet->packet[2] & 0x80) && packet->length != 5) {
		// too long control frame
		ESP_LOGE(TAG, "tx too-long control packet, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if (packet->length == 6) {
		// impossible packet length
		ESP_LOGE(TAG, "tx impossible packet length, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if (packet->length >= 7 && (((packet->packet[3] & 0x3) << 8) | packet->packet[4]) != packet->length - 5) {
		// packet length does not match claimed length
		ESP_LOGE(TAG, "tx length field (%d) does not match actual packet length (%d)", (((packet->packet[3] & 0x3) << 8) | packet->packet[4]), packet->length - 5);
		flash_led_once(LT_RED_LED);
		return false;
	}

	// check the CRC
	crc_state_t crc;
	crc_state_init(&crc);
	crc_state_append_all(&crc, packet->packet, packet->length);
	if (!crc_state_ok(&crc)) {
		ESP_LOGE(TAG, "bad CRC on tx: IP bug?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	return true;
}*/
