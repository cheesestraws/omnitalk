#include "net/net.h"

#include <esp_event.h>
#include <esp_netif.h>
#include <esp_netif_types.h>

#include "lap/sink/sink.h"
#include "net/ethernet/ethernet.h"
#include "net/ltoudp/ltoudp.h"
#include "net/mdns.h"
#include "net/tashtalk/tashtalk.h"

void start_net(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	start_ethernet();
	start_mdns();
	start_tashtalk();
	start_ltoudp();
	
	/* start_sink("SINK-eth", ethertalkv2_get_transport());
	start_sink("SINK-tt", tashtalk_get_transport());
	start_sink("SINK-ltoudp", ltoudp_get_transport()); */
}