#include "net/common.h"

#include <stdlib.h>

#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_netif_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#define BASE_HOSTNAME "omnitalk"

esp_netif_t* _Atomic active_ip_net_if;

// ip_ready_event fires when we get an IP address, and things
// that need IP can wake up
static EventGroupHandle_t ip_ready_event;


char *generate_hostname(void) {
	uint8_t mac[6];
	char   *hostname;
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", BASE_HOSTNAME, mac[3], mac[4], mac[5])) {
		abort();
	}

	return hostname;
}

void wait_for_ip_ready(void) {
	xEventGroupWaitBits(
		ip_ready_event,
		1,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY
	);
}

void mark_ip_ready(void) {
	xEventGroupSetBits(ip_ready_event, 1);
}

void start_common(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	
	ip_ready_event = xEventGroupCreate();
}
