#include "net/net.h"

#include <esp_event.h>
#include <esp_netif.h>
#include <esp_netif_types.h>

#include "net/ethernet.h"
#include "net/mdns.h"

void start_net(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	start_ethernet();
	start_mdns();
}