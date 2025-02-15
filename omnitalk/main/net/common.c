#include "net/common.h"

#include <stdlib.h>

#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

#define BASE_HOSTNAME "omnitalk"

esp_netif_t* _Atomic active_ip_net_if;
_Atomic bool ip_ready;

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
	while (1) {
		if (ip_ready) { return; }
		
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
