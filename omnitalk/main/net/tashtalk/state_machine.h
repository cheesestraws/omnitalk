#pragma once

#include <stdatomic.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "mem/buffers.h"
#include "util/crc.h"   
   
/* a tashtalk_rx_state_t value represents a state machine for getting LLAP 
   packets out of the byte stream from TashTalk. */
typedef struct {
	_Atomic bool send_output_to_queue;
	buffer_t* packet_in_progress; // the buffer that holds the in-flight packet
	QueueHandle_t output_queue;
	bool in_escape;
	crc_state_t crc;
} tashtalk_rx_state_t;

tashtalk_rx_state_t* new_tashtalk_rx_state(QueueHandle_t output_queue);
void tashtalk_feed(tashtalk_rx_state_t* state, unsigned char byte);
void tashtalk_feed_all(tashtalk_rx_state_t* state, unsigned char* buf, int count);

// tashtalk_tx_validate checks whether an LLAP packet is valid or whether
// tashtalk will choke on it (i.e. are the lengths right etc)
//bool tashtalk_tx_validate(llap_packet* packet);
