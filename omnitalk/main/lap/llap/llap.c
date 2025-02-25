#include "lap/llap/llap.h"

#include <stdbool.h>
#include <stdlib.h>

#include <esp_log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <lwip/inet.h>

#include "lap/id.h"
#include "lap/lap.h"
#include "mem/buffers.h"
#include "net/transport.h"
#include "proto/ddp.h"
#include "proto/llap.h"
#include "proto/rtmp.h"
#include "util/require/goto.h"
#include "web/stats.h"
#include "runloop.h"

static const char* TAG = "LLAP";

static bool llap_extract_ddp_packet(buffer_t *buf) {
	llap_hdr_t *llap_hdr;

	// Does the packet have room for an LLAP header?
	if (buf->length < sizeof(llap_hdr_t)) {
		return false;
	}
	llap_hdr = (llap_hdr_t*)buf->data;
	
	// Yep - is it a ddp packet?
	// For short headers, it's sizeof(ddp_short_header_t) not
	// sizeof(llap_hdr_t) + sizeof(ddp_short_header_t) because
	// the short ddp header subsumes the llap header entirely.
	if (buf->length > sizeof(ddp_short_header_t) &&
	    llap_hdr->llap_type == LLAP_TYPE_DDP_SHORT) {
	    
		buf->ddp_type = BUF_SHORT_HEADER;
		buf->ddp_length = buf->length;
		buf->ddp_capacity = buf->capacity;
		buf->ddp_data = buf->data;
		buf->ddp_payload = buf->ddp_data + sizeof(ddp_short_header_t);
		buf->ddp_payload_length = buf->ddp_length - sizeof(ddp_short_header_t);
	} else if (buf->length > sizeof(llap_hdr_t) + sizeof(ddp_long_header_t) &&
	           llap_hdr->llap_type == LLAP_TYPE_DDP_LONG) {
		buf->ddp_type = BUF_LONG_HEADER;
		buf->ddp_length = buf->length - 3;
		buf->ddp_capacity = buf->capacity - 3;
		buf->ddp_data = buf->data + 3;
		buf->ddp_payload = buf->ddp_data + sizeof(ddp_long_header_t);
		buf->ddp_payload_length = buf->ddp_length - sizeof(ddp_long_header_t);
	} else {
		return false;
	}
	
	buf->ddp_ready = true;
	return true;
}

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
		
			// bummer, someone's got our address.  try again with another.
			got_ack = true;
		
		ack_loop_continue:
			free(ack_buffer);
		}
	} while (got_ack);
	
	ESP_LOGI(TAG, "got address %d", (int)candidate);
	
	info->node_addr = candidate;
	ESP_ERROR_CHECK(set_transport_node_address(transport, candidate));
	stats_lap_metadata[lap->id].node_address=candidate;
	stats_lap_metadata[lap->id].state="acquiring network info";
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
			ESP_LOGE(TAG, "couldn't send rtmp");
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
		
		if (!llap_extract_ddp_packet(rtmp_resp)) {
			goto discard;
		}
		
		// Is the payload long enough?
		if (rtmp_resp->ddp_payload_length < sizeof(rtmp_response_t)) {
			goto discard;
		}
		
		REQUIRE(DDP_DST(rtmp_resp) == info->node_addr, discard);
		REQUIRE(DDP_DSTSOCK(rtmp_resp) == 1, discard);
		REQUIRE(DDP_SRCSOCK(rtmp_resp) == 1, discard);
		REQUIRE(DDP_TYPE(rtmp_resp) == 1, discard);

		// Playing a bit fast and loose, we'll ignore the datagram length,
		// we've already checked the packet buffer size above and meh.
		rtmp_response_t *body = (rtmp_response_t*)(rtmp_resp->ddp_payload);
		
		// It's addressed to us, and it claims to be an RTMP response.  Hooray
		if (body->id_length_bits != 8) {
			ESP_LOGE(TAG, "got rtmp reply with invalid id_length_bits, wut?");
			// TODO: tick stats up here
			goto discard;
		}
		
		info->discovered_net = ntohs(body->senders_network);
		info->discovered_seeding_node = body->node_id;
		free(rtmp_resp);
		break;
	
	discard:
		free(rtmp_resp);
	}
	
	ESP_LOGI(TAG, "[%s] got network 0x%x (%d)", lap->name, (int)info->discovered_net,  (int)info->discovered_net);
	
	stats_lap_metadata[lap->id].discovered_network=info->discovered_net;
	stats_lap_metadata[lap->id].state="running";
	
	lap->my_address = info->node_addr;
	lap->my_network = info->discovered_net;
	lap->network_range_start = info->discovered_net;
	lap->network_range_end = info->discovered_net;
	
	info->state = LLAP_RUNNING;
}

void llap_run_for_a_while(lap_t *lap) {
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;
	buffer_t *recvbuf;
	
	int64_t start_time = esp_timer_get_time();
	while (esp_timer_get_time() < start_time + 15000000) {
		recvbuf = trecv_with_timeout(transport, 1000 / portTICK_PERIOD_MS);
		if (recvbuf == NULL) {
			continue;
		}
		
		// update the receive chain for the packet so we know where it came from
		recvbuf->recv_chain.transport = transport;
		recvbuf->recv_chain.lap = lap;
		
		llap_hdr_t *hdr = ((llap_hdr_t*)recvbuf->data);
		
		// is this an ENQ?
		if (recvbuf->length == 3 && hdr->llap_type == LLAP_TYPE_ENQ && hdr->dst == info->node_addr) {
			ESP_LOGI(TAG, "someone's trying to steal our address, we should do something about that");
			
			// turn our received ENQ into an ACK
			hdr->llap_type = LLAP_TYPE_ACK;
			BaseType_t err = xQueueSendToFront(transport->outbound, &recvbuf, portMAX_DELAY);
			if (err == pdTRUE) {
				continue;
			} else {
				ESP_LOGE(TAG, "failed to push ack");
				goto discard;
			}
		}
		
		// Extract DDP packet
		if (!llap_extract_ddp_packet(recvbuf)) {
			goto discard;
		}
		
		ESP_LOGI(TAG, "src %d dst %d", (int)DDP_SRC(recvbuf), (int)DDP_DST(recvbuf));
		
		if (ddp_packet_is_mine(lap, recvbuf)) {
			ESP_LOGI(TAG, "packet is addressed to me!");
			printbuf(recvbuf);
			// do something
			
			if (rlsend(lap->controlplane, recvbuf)) {
				continue;
			} else {
				stats.controlplane_inbound_queue_full++;
			}
		}
		
	discard:
		// TODO: tick stat up
		freebuf(recvbuf);
	}
}

void llap_inbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;
	llap_info_t *info = (llap_info_t*)lap->info;
	
	wait_for_transport_ready(transport);
	
	while(1) {
		if (info->state == LLAP_ACQUIRING_ADDRESS) {
			llap_acquire_address(lap);
			continue;
		}
		if (info->state == LLAP_ACQUIRING_NETINFO) {
			llap_acquire_netinfo(lap);
			continue;
		}
		if (info->state == LLAP_RUNNING) {
			llap_run_for_a_while(lap);
		}
	}
}

void llap_outbound_runloop(void* lapParam) {
	lap_t *lap = (lap_t*)lapParam;
	transport_t *transport = lap->transport;

	wait_for_transport_ready(transport);

	vTaskDelay(portMAX_DELAY);
}


lap_t *start_llap(char* name, transport_t *transport, runloop_info_t *controlplane, runloop_info_t *dataplane) {
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
	lap->id = get_next_lap_id();
	lap->info = (void*)info;
	lap->name = name;
	lap->transport = transport;
	
	lap->controlplane = controlplane;
	lap->dataplane = dataplane;
	
	// enable metadata metric
	stats_lap_metadata[lap->id].name = name;
	stats_lap_metadata[lap->id].state="acquiring address";
	stats_lap_metadata[lap->id].ok = true;
	
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
