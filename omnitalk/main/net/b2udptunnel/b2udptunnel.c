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
#include "proto/SNAP.h"
#include "web/stats.h"

static char* TAG="b2eth";

static _Atomic int b2_udp_sock = -1;
static _Atomic bool b2_transport_enabled = false;

QueueHandle_t b2_outbound_queue = NULL;
QueueHandle_t b2_inbound_queue = NULL;


static void init_b2udptunnel(void) {
	int sock = -1;
	struct sockaddr_in saddr = { 0 };
	
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket() failed.  error: %d", errno);
		return;
	}

	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(6066);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (err < 0) {
		ESP_LOGE(TAG, "bind() failed, error: %d", errno);
		goto err_cleanup;
	}
	
	int bc = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(int));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...SO_BROADCAST...) failed, error: %d", errno);
		goto err_cleanup;
	}

	
err_cleanup:
	close(sock);
	return;
}

static bool sanity_check_incoming_frame(buffer_t *buf) {
	// Does the frame actually have room for ethernet and SNAP
	// headers?
	if (buf->length < sizeof(struct eth_hdr) + sizeof(snap_hdr_t)) {
		stats.b2eth_err_rx_frame_too_short++;
		return false;
	}
	
	struct eth_hdr *hdr = (struct eth_hdr*)buf->data;
	// Do we have a 'b2' source MAC?
	if (hdr->src.addr[0] != 'B' || hdr->src.addr[1] != '2') {
		stats.b2eth_err_rx_invalid_src_mac++;
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
		struct sockaddr_in cliaddr = { 0 };
		socklen_t clilen = sizeof(cliaddr);
		while (1) {
			buf = newbuf(ETHERNET_FRAME_LEN);
			
			int len = recvfrom(b2_udp_sock, buf->data, buf->capacity, 0, 
				(struct sockaddr *)&cliaddr, &clilen);
			if (len == 0) {
				goto free_and_continue;
			}
						
			if (len < 0) {
				stats.b2eth_err_rx_recvfrom_failed++;
				goto free_and_continue;
			}
			
			buf->length = len;
			
			if (!sanity_check_incoming_frame(buf)) {
				goto free_and_continue;
			}		
				
			if (b2_transport_enabled) {
				BaseType_t err = xQueueSendToBack(b2_inbound_queue, &buf, (TickType_t)0);
				if (err != pdTRUE) {
					stats.ltoudp_err_rx_queue_full++;
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

}

void start_b2udptunnel(void) {
	
}
