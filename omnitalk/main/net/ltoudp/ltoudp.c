#include "net/ltoudp/ltoudp.h"

#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_types.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include "mem/buffers.h"
#include "net/common.h"
#include "web/stats.h"

static const char* TAG = "LTOUDP";

static _Atomic int udp_sock = -1;
TaskHandle_t udp_rx_task = NULL;
TaskHandle_t udp_tx_task = NULL;

QueueHandle_t ltoudp_outgoing_queue = NULL;
QueueHandle_t ltoudp_incoming_queue = NULL;

void init_udp(void) {
	struct sockaddr_in saddr = { 0 };
	int sock = -1;
	int err = 0;
	struct in_addr outgoing_addr = { 0 };
	esp_netif_ip_info_t ip_info = { 0 };
	
	/* bail out early if we don't have a wifi network interface yet */
	
	if (active_ip_net_if == NULL) {
		ESP_LOGW(TAG, "no underlying network interface yet.");
		return;
	}
		
	/* or if we can't get its IP */
	err = esp_netif_get_ip_info(active_ip_net_if, &ip_info);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "network interface wouldn't tell us its IP.");
		return;
	}
	
	inet_addr_from_ip4addr(&outgoing_addr, &ip_info.ip);
	
	/* create the multicast socket and twiddle its socket options */

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket() failed.  error: %d", errno);
		return;
	}
	
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(1954);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (err < 0) {
		ESP_LOGE(TAG, "bind() failed, error: %d", errno);
		goto err_cleanup;
	}
		
	uint8_t ttl = 1;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_MULTICAST_TTL...) failed, error: %d", errno);
		goto err_cleanup;
	}
		
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &outgoing_addr, sizeof(struct in_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_MULTICAST_IF...) failed, error: %d", errno);
		goto err_cleanup;
	}


	struct ip_mreq imreq = { 0 };
	imreq.imr_interface.s_addr = IPADDR_ANY;
	err = inet_aton("239.192.76.84", &imreq.imr_multiaddr.s_addr);
	if (err != 1) {
		ESP_LOGE(TAG, "inet_aton failed, error: %d", errno);
		goto err_cleanup;
	}

	err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						 &imreq, sizeof(struct ip_mreq));
	if (err < 0) {
		ESP_LOGE(TAG, "setsockopt(...IP_ADD_MEMBERSHIP...) failed, error: %d", errno);
		goto err_cleanup;
	}
	 
	udp_sock = sock;
	
	ESP_LOGI(TAG, "multicast socket now ready.");
						
	return;

	
err_cleanup:
	close(sock);
	return;

}

void udp_rx_runloop(void *pvParameters) {
	ESP_LOGI(TAG, "starting LToUDP listener");
	init_udp();
	
	while(1) {
		/* retry if we need to */
		while(udp_sock == -1) {
			ESP_LOGE(TAG, "setting up LToUDP listener failed.  Retrying...");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			init_udp();
		}
	
		while(1) {
			unsigned char buffer[603+4]; // 603 for LLAP (per Inside Appletalk) + 
										 // 4 bytes LToUDP header
			int len = recv(udp_sock, buffer, sizeof(buffer), 0);
			if (len < 0) {
				stats.ltoudp_err_rx_recv_error++;
				break;
			}
			if (len > 609) {
				stats.ltoudp_err_rx_packet_too_long++;
				ESP_LOGE(TAG, "packet too long: %d", len);
				continue;
			}
			if (len > 7) {
				stats.ltoudp_rx_frames++;
				
				// fetch an empty buffer from the pool and fill it with
				// packet info
				// length is wrong but it'll do for now
				buffer_t *buf = newbuf(ETHERNET_FRAME_LEN);
				if (buf != NULL) {				
					// copy the LLAP packet into the packet buffer
					buf->length = len-4; // -4 for LToUDP tag
					memcpy(buf->data, buffer+4, len-4);
				
					BaseType_t err = xQueueSendToBack(ltoudp_incoming_queue, &buf, (TickType_t)0);
					if (err != pdTRUE) {
						stats.ltoudp_err_rx_queue_full++;
						freebuf(buf);
					}
				}
			}
		}
	
		close(udp_sock);
		udp_sock = -1;
	}
}

void udp_tx_runloop(void *pvParameters) {
	buffer_t* packet = NULL;
	unsigned char outgoing_buffer[605 + 4] = { 0 }; // LToUDP has 4 byte header
	struct sockaddr_in dest_addr = {0};
	int err = 0;
		
	dest_addr.sin_addr.s_addr = inet_addr("239.192.76.84");
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(1954);
	
	ESP_LOGI(TAG, "starting LToUDP sender");
	
	while(1) {
		xQueueReceive(ltoudp_outgoing_queue, &packet, portMAX_DELAY);
				
		if (packet == NULL) {
			continue;
		}
		   
		if (udp_sock != -1) {
			memcpy(outgoing_buffer+4, packet->data, packet->length);
			
			err = sendto(udp_sock, outgoing_buffer, packet->length+4, 0, 
				(struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "error: sendto: errno %d", errno);
				stats.ltoudp_err_tx_send_error++;
			}
			err = 0;
		}
				
		freebuf(packet);
	}
}


void start_ltoudp(void) {
	ltoudp_outgoing_queue = xQueueCreate(LTOUDP_QUEUE_DEPTH, sizeof(buffer_t*));;
	ltoudp_incoming_queue = xQueueCreate(LTOUDP_QUEUE_DEPTH, sizeof(buffer_t*));;
	xTaskCreate(&udp_rx_runloop, "LToUDP-rx", 4096, NULL, 5, &udp_rx_task);
	xTaskCreate(&udp_tx_runloop, "LToUDP-tx", 4096, NULL, 5, &udp_tx_task);

}
