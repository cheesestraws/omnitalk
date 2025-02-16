#include "lap/llap/llap.h"

#include <stdlib.h>

#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <lwip/inet.h>

#include "lap/lap.h"
#include "mem/buffers.h"
#include "proto/ddp.h"
#include "proto/llap.h"
#include "proto/rtmp.h"
#include "util/require/goto.h"

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
		
		ESP_LOGI(TAG, "addr acq candidate %d", (int)candidate);
				
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
		while (esp_timer_get_time() < start_time + 100000) {
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
		
			ESP_LOGI(TAG, "got ack for %d", (int)candidate);
			// bummer, someone's got our address.  try again with another.
			got_ack = true;
		
		ack_loop_continue:
			free(ack_buffer);
		}
	} while (got_ack);
	
	ESP_LOGI(TAG, "got address %d", (int)candidate);
	
	info->node_addr = candidate;
	info->state = LLAP_ACQUIRING_NETINFO;
}

void llap_acquire_netinfo(lap_t *lap) {
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;

	buffer_t *rtmp_req;
	buffer_t *rtmp_resp;

	// Send a few RTMP requests.  See Inside Appletalk p5-17 et seq
	for (int i = 0; i < 5; i++) {
		rtmp_req = newbuf(sizeof(ddp_short_header_t) + 3);
		rtmp_req->length = rtmp_req->capacity - 2;
		
		ddp_short_header_t* hdr = (ddp_short_header_t*)rtmp_req->data;
		
		hdr->dst = DDP_ADDR_BROADCAST; // broadcast
		hdr->src = info->node_addr;
		hdr->always_one = 1;
		hdr->datagram_length = htons(6);
		hdr->dst_sock = DDP_SOCKET_RTMP;
		hdr->src_sock = DDP_SOCKET_RTMP;
		hdr->ddp_type = 5;
		hdr->body[0] = 1;
	
		if (!tsend_and_block(transport, rtmp_req)) {
			// TODO: stat tickup
			freebuf(rtmp_req);
		}
	}

	// Wait for reply
	int64_t start_time = esp_timer_get_time();
	while (esp_timer_get_time() < start_time + 1000000) {
		rtmp_resp = trecv_with_timeout(transport, 1);
		if (rtmp_resp == NULL) {
			continue;
		}
	
		ESP_LOGI(TAG, "[%s] got %d bytes", lap->name, (int)rtmp_resp->length);
	
		if (rtmp_resp->length < sizeof(ddp_short_header_t) + sizeof(rtmp_response_t)) {
			goto rtmp_resp_loop_continue;
		}
		
		ddp_short_header_t* hdr = (ddp_short_header_t*)rtmp_resp->data;
		rtmp_response_t* body = (rtmp_response_t*)(&hdr->body[0]);
		
		// Is this an RTMP response we asked for?
		REQUIRE(hdr->dst == info->node_addr, rtmp_resp_loop_continue);
		REQUIRE(hdr->dst_sock == 1, rtmp_resp_loop_continue);
		REQUIRE(hdr->src_sock == 1, rtmp_resp_loop_continue);
		REQUIRE(hdr->ddp_type == 1, rtmp_resp_loop_continue);
		// Playing a bit fast and loose, we'll ignore the datagram length,
		// we've already checked the packet buffer size above and meh.
		
		// It's addressed to us, and it claims to be an RTMP response.  Hooray
		if (body->id_length_bits != 8) {
			ESP_LOGE(TAG, "got rtmp reply with invalid id_length_bits, wut?");
			// TODO: tick stats up here
			goto rtmp_resp_loop_continue;
		}
		
		info->discovered_net = ntohs(body->senders_network);
		free(rtmp_resp);
		break;
	
	rtmp_resp_loop_continue:
		free(rtmp_resp);
	}
	
	ESP_LOGI(TAG, "got network 0x%x (%d)", (int)info->discovered_net,  (int)info->discovered_net);
	
	info->state = LLAP_RUNNING;
}

void llap_inbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;
	
	wait_for_transport_ready(transport);
	ESP_LOGI(TAG, "transport ready, inbound llap is go");
	
	while(1) {
		if (info->state == LLAP_ACQUIRING_ADDRESS) {
			llap_acquire_address(lap);
			continue;
		}
		if (info->state == LLAP_ACQUIRING_NETINFO) {
			llap_acquire_netinfo(lap);
			continue;
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void llap_outbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;

	wait_for_transport_ready(transport);
	ESP_LOGI(TAG, "transport ready, outbound llap is go");

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
	
	enable_transport(transport);
	
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
