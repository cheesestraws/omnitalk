#include "net/b2udptunnel/b2udptunnel.h"

#include <stdbool.h>

#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_types.h>
#include <lwip/prot/ethernet.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include "mem/buffers.h"
#include "net/common.h"
#include "net/transport.h"
#include "proto/SNAP.h"
#include "web/stats.h"
#include "tunables.h"

static char* TAG="b2eth";

static _Atomic int b2_udp_sock = -1;
static _Atomic bool b2_transport_enabled = false;
static transport_t b2_transport;

QueueHandle_t b2_outbound_queue = NULL;
QueueHandle_t b2_inbound_queue = NULL;
TaskHandle_t b2_outbound_task = NULL;
TaskHandle_t b2_inbound_task = NULL;

static bool macaddr_is_appletalk_broadcast(uint8_t *addr) {
	return addr[0] == 0x09 && addr[1] == 0x00 && addr[2] == 0x07 && addr[3] == 0xff && addr[4] == 0xff && addr[5] == 0xff;
}

static bool macaddr_is_ethernet_broadcast(uint8_t *addr) {
	return addr[0] == 0xff && addr[1] == 0xff && addr[2] == 0xff  && addr[3] == 0xff && addr[4] == 0xff && addr[5] == 0xff;
}

static bool macaddr_is_b2_unicast(uint8_t *addr) {
	return addr[0] == 'B' && addr[1] == '2';
}

static void init_b2udptunnel(void) {
	int sock = -1;
	struct sockaddr_in saddr = { 0 };
	
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket() failed.  error: %d", errno);
		return;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(6066);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (err < 0) {
		ESP_LOGE(TAG, "bind() failed, error: %d", errno);
		goto err_cleanup;
	}
	
	socklen_t bc = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...SO_BROADCAST...) failed, error: %d", errno);
		goto err_cleanup;
	}

	b2_udp_sock = sock;
	mark_transport_ready(&b2_transport);
	
	return;
	
err_cleanup:
	close(sock);
	return;
}

static bool sanity_check_incoming_frame(buffer_t *buf) {
	// Does the frame actually have room for ethernet and SNAP
	// headers?
	if (buf->length < sizeof(struct eth_hdr) + sizeof(snap_hdr_t)) {
		stats.transport_in_errors__transport_b2udp__err_frame_too_short++;
		return false;
	}
	
	struct eth_hdr *hdr = (struct eth_hdr*)buf->data;
	// Do we have a 'b2' source MAC?
	if (hdr->src.addr[0] != 'B' || hdr->src.addr[1] != '2') {
		stats.transport_in_errors__transport_b2udp__err_invalid_source_MAC++;
		return false;
	}

	return true;
}

static void b2udptunnel_inbound_runloop(void* dummy) {
	init_b2udptunnel();
	
	while (1) {
		while(b2_udp_sock == -1) {
			ESP_LOGE(TAG, "setting up b2udptunnel listener failed.  Retrying...");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			init_b2udptunnel();
		}

		buffer_t *buf = NULL;
		struct sockaddr_storage cliaddr = { 0 };
		socklen_t clilen = sizeof(cliaddr);
		while (1) {
			buf = newbuf(ETHERNET_FRAME_LEN, sizeof(struct eth_hdr) + sizeof(snap_hdr_t));
			
			int len = recvfrom(b2_udp_sock, buf->data, buf->capacity, 0,
				(struct sockaddr*)&cliaddr, &clilen);
				
			if (len == 0) {
				goto free_and_continue;
			}
						
			if (len < 0) {
				stats.transport_in_errors__transport_b2udp__err_recvfrom_failed++;
				goto free_and_continue;
			}
			
			stats.transport_in_octets__transport_b2udp += len;
			stats.transport_in_frames__transport_b2udp++;
			
			// TODO: sanity check sender IP address
						
			buf->length = len;
						
			if (!sanity_check_incoming_frame(buf)) {
				goto free_and_continue;
			}		
				
			if (b2_transport_enabled) {
				BaseType_t err = xQueueSendToBack(b2_inbound_queue, &buf, (TickType_t)0);
				if (err != pdTRUE) {
					stats.transport_in_errors__transport_b2udp__err_lap_queue_full++;
					goto free_and_continue;
				}
			} else {
				goto free_and_continue;
			}
			
			continue;
			
		free_and_continue:
			freebuf(buf);
		}
	}
}

static void b2udptunnel_outbound_runloop(void* dummy) {
	buffer_t *buf;
	struct sockaddr_in dest_addr = {0};
	
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(6066);

	while(1) {
		xQueueReceive(b2_outbound_queue, &buf, portMAX_DELAY);
		if (buf == NULL) {
			continue;
		}
		
		if (b2_udp_sock == -1) {
			goto skip_processing;
		}
		
		// Is the packet long enough?
		if (buf->length < sizeof(struct eth_hdr) + sizeof(snap_hdr_t)) {
			stats.transport_out_errors__transport_b2udp__err_frame_too_short++;
			goto skip_processing;
		}
		
		// Work out destination IP
		struct eth_hdr *hdr = (struct eth_hdr*)buf->data;
		if (macaddr_is_appletalk_broadcast(hdr->dest.addr) ||
			macaddr_is_ethernet_broadcast(hdr->dest.addr)) {
		
			dest_addr.sin_addr.s_addr = INADDR_BROADCAST;
		} else if (macaddr_is_b2_unicast(hdr->dest.addr)) {
			uint32_t dest = (hdr->dest.addr[2] << 24) | (hdr->dest.addr[3] << 16) | 
			                (hdr->dest.addr[4] << 8) | hdr->dest.addr[5];
			dest_addr.sin_addr.s_addr = htonl(dest);
		} else {
			stats.transport_out_errors__transport_b2udp__err_invalid_dst_MAC++;
			goto skip_processing;
		}
		
		if (sendto(b2_udp_sock, buf, buf->length, 0,
			(struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
		
			stats.transport_out_errors__transport_b2udp__err_sendto_failed++;
		} else {
			stats.transport_out_octets__transport_b2udp+=buf->length;
			stats.transport_out_frames__transport_b2udp++;
		}
		
	skip_processing:
		freebuf(buf);
	}
}

void start_b2udptunnel(void) {
	b2_transport.ready_event = xEventGroupCreate();
	b2_outbound_queue = xQueueCreate(B2UDPTUNNEL_QUEUE_DEPTH, sizeof(buffer_t*));;
	b2_inbound_queue = xQueueCreate(B2UDPTUNNEL_QUEUE_DEPTH, sizeof(buffer_t*));;
	xTaskCreate(&b2udptunnel_inbound_runloop, "B2UDP-rx", 4096, NULL, 5, &b2_inbound_task);
	xTaskCreate(&b2udptunnel_outbound_runloop, "B2UDP-tx", 4096, NULL, 5, &b2_outbound_task);
}

esp_err_t b2_transport_enable(transport_t* dummy) {
	b2_transport_enabled = true;
	return ESP_OK;
}

esp_err_t b2_transport_disable(transport_t* dummy) {
	b2_transport_enabled = false;
	return ESP_OK;
}

static transport_t b2_transport = {
	.quality = QUALITY_B2ETH,

	.kind = "b2",
	.private_data = NULL,
	
	.enable = &b2_transport_enable,
	.disable = &b2_transport_disable,
};

transport_t* b2_get_transport(void) {
	// warn if we do something silly and attempt to get multiple
	// transports for this interface
	static int attempts = 0;
	if (attempts > 0) {
		ESP_LOGE(TAG, "multiple transports requested for b2; beware - there can be only one!");
	}
	attempts++;
	
	b2_transport.inbound = b2_inbound_queue;
	b2_transport.outbound = b2_outbound_queue;
	
	return &b2_transport;
	
}

