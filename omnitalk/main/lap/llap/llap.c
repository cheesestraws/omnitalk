#include "lap/llap/llap.h"

#include <stdlib.h>

#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/llap.h"

static const char* TAG = "LLAP";

void llap_acquire_address(lap_t *lap) {
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;
	buffer_t *enq_buffer = NULL;
	buffer_t *ack_buffer = NULL;
	
	ESP_LOGI(TAG, "starting address acquisition");
	
	uint8_t candidate;
	bool got_ack;

	do {
		got_ack = false;
		
		// Pick a random address
		candidate = (uint8_t)(esp_random() % 127);
		candidate += 128; // server addresses go 128 and above
				
		// send a burst of ENQs
		for (int i = 0; i < 10; i++) {
			enq_buffer = newbuf(5);
			enq_buffer->length = 3;
			((llap_hdr_t*)enq_buffer->data)->src = candidate;
			((llap_hdr_t*)enq_buffer->data)->dst = candidate;
			((llap_hdr_t*)enq_buffer->data)->llap_type = LLAP_TYPE_ENQ;
		
			if (!tsend_and_block(transport, enq_buffer)) {
				// TODO: stat tickup
				freebuf(enq_buffer);
			}
		}
	
	
		// Wait for reply
		int64_t start_time = esp_timer_get_time();
		while (esp_timer_get_time() < start_time + 10000) {
			ack_buffer = trecv_with_timeout(transport, 1);
			if (ack_buffer == NULL) {
				continue;
			}
		
			// Is the packet we got actually an ack?
		
			if (ack_buffer->length != 3) {
				goto ack_loop_continue;
			}
			llap_hdr_t *hdr = (llap_hdr_t*)ack_buffer->data;
		
			if (hdr->llap_type != LLAP_TYPE_ACK || hdr->src != candidate ||
				hdr->dst != candidate) {
		
				goto ack_loop_continue;	
			}
		
			// bummer, someone's got our address.  try again with another.
			got_ack = true;
		
		ack_loop_continue:
			free(ack_buffer);
		}
	} while (got_ack);
	
	info->node_addr = candidate;
	info->state = LLAP_ACQUIRING_NETINFO;
}

void llap_inbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;

	vTaskDelay(portMAX_DELAY);
	
	while(1) {
		if (info->state == LLAP_ACQUIRING_ADDRESS) {
			llap_acquire_address(lap);
			continue;
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void llap_outbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;


	vTaskDelay(portMAX_DELAY);
}


lap_t *start_llap(char* name, transport_t *transport) {
	lap_t *lap = calloc(1, sizeof(lap_t));
	if (lap == NULL) {
		return NULL;
	}
	
	llap_info_t *info = calloc(1, sizeof(llap_info_t));
	if (info == NULL) {
		free(lap);
		return NULL;
	}
	
	// fill in LAP fields
	lap->info = (void*)info;
	lap->name = name;
	lap->transport = transport;
	
	// start runloop
	char* task_name;
	asprintf(&task_name, "llap:%s:inbound", name);
	xTaskCreate(&llap_inbound_runloop, task_name, 2048, (void*)lap, 5, NULL);
	// do NOT free task_name, freertos will be holding onto a reference to it
	asprintf(&task_name, "llap:%s:outbound", name);
	xTaskCreate(&llap_outbound_runloop, task_name, 2048, (void*)lap, 5, NULL);
	// freertos will hold onto a reference to this string, too

	return lap;
}