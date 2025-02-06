#include "net/ethernet_output.h"

#include <stddef.h>

#include <esp_err.h>
#include <esp_eth.h>
#include <esp_netif.h>
#include <esp_netif_types.h>

#include "web/stats.h"


esp_err_t send_ethernet(esp_eth_handle_t hdl, void *buf, size_t length) {
	stats.eth_output_path_ifOutOctets += length;

	return esp_eth_transmit(hdl, buf, length);
}

// a free shim, needed for the interface configuration evil
static void eth_l2_free(void *h, void* buffer)
{
    free(buffer);
}

void munge_ethernet_output_path(esp_eth_handle_t eth_driver, esp_netif_t *esp_netif) {
	// Replace the interface configuration with one that contains
	// our transmit function.
	
	esp_netif_driver_ifconfig_t driver_ifconfig = {
		.handle =  eth_driver,
        .transmit = send_ethernet,
        .driver_free_rx_buffer = eth_l2_free
	};
	
	ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));
}
