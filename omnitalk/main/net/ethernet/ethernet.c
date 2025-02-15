#include "net/ethernet/ethernet.h"

#include <stdatomic.h>
#include <stdbool.h>

#include <esp_err.h>
#include <esp_eth.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_types.h>
#include <driver/gpio.h>
#include <lwip/prot/ethernet.h>
#include <lwip/def.h>

#include "mem/buffers.h"
#include "net/ethernet/ethernet_output.h"
#include "net/common.h"
#include "net/transport.h"
#include "proto/SNAP.h"
#include "web/stats.h"
#include "hw.h"

#define REQUIRE(x) if(!(x)) { return false; }

_Atomic bool ethernet_transport_enabled = false;

TaskHandle_t ethertalkv2_inbound_task = NULL;
TaskHandle_t ethertalkv2_outbound_task = NULL;
QueueHandle_t ethertalkv2_inbound_queue = NULL;
QueueHandle_t ethertalkv2_outbound_queue = NULL;

static const char* TAG = "ETHERNET";

bool is_appletalk_frame(uint8_t *buffer, uint32_t length) {
	// We need to have both an ethernet header and a SNAP header.
	// First check our length.
	REQUIRE(length >= sizeof(struct eth_hdr) + sizeof(snap_hdr_t));
	
	// We should have a length, not an ethertype.  I don't think
	// any AppleTalk stuff supports jumbo frames, so we can do this
	// the easy way: a type or size <= 1500 is a length, higher is
	// an ethertype.
	struct eth_hdr *eth_hdr = (struct eth_hdr*)buffer;
	REQUIRE(ntohs(eth_hdr->type) <= 1500)
		
	// The LLC header needs to contain the SNAP SAPs
	snap_hdr_t *snap_hdr = GET_SNAP_HDR(buffer);
		
	REQUIRE(snap_hdr->dest_sap == 0xAA &&
		snap_hdr->src_sap == 0xAA &&
		snap_hdr->control_byte == 3);
				
	// And the SNAP protocol discriminator needs to be right
	REQUIRE(snap_hdr->proto_discriminator_top_byte == 0x08 &&
		snap_hdr->proto_discriminator_bottom_bytes == PP_HTONL(0x0007809B));
			
	return true;
}

bool is_aarp_frame(uint8_t *buffer, uint32_t length) {
	// We need to have both an ethernet header and a SNAP header.
	// First check our length.
	REQUIRE(length >= sizeof(struct eth_hdr) + sizeof(snap_hdr_t));
	
	// We should have a length, not an ethertype.  I don't think
	// any AppleTalk stuff supports jumbo frames, so we can do this
	// the easy way: a type or size <= 1500 is a length, higher is
	// an ethertype.
	struct eth_hdr *eth_hdr = (struct eth_hdr*)buffer;
	REQUIRE(ntohs(eth_hdr->type) <= 1500)
	
	
	// The LLC header needs to contain the SNAP SAPs
	snap_hdr_t *snap_hdr = GET_SNAP_HDR(buffer);
	REQUIRE(snap_hdr->dest_sap == 0xAA &&
		snap_hdr->src_sap == 0xAA &&
		snap_hdr->control_byte == 3);
		
	// And the SNAP protocol discriminator needs to be right
	REQUIRE(snap_hdr->proto_discriminator_top_byte == 0x00 &&
		snap_hdr->proto_discriminator_bottom_bytes == PP_HTONL(0x000080F3));
	
	return true;
}

// ethernet_input_path is the callback that will get called whenever
// we get a packet.  For most things, it just passes it straight through
// to esp_netif.
//
// Technically this is a bit naughty and we should do this with a tap,
// but I'm too tired.
esp_err_t ethernet_input_path(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t length, void *priv) {
	// Let's not break L2 TAP
#if CONFIG_ESP_NETIF_L2_TAP
    esp_err_t ret = ESP_OK;
    ret = esp_vfs_l2tap_eth_filter_frame(eth_handle, buffer, (size_t *)&length, info);
    if (length == 0) {
        return ret;
    }
#endif

	stats.transport_in_octets__transport_ethernet += (unsigned long)length;
	stats.transport_in_frames__transport_ethernet++;

	// Is this an AppleTalk frame?
	if (is_appletalk_frame(buffer, length)) {
		ESP_LOGI(TAG, "got appletalk frame");
		stats.eth_recv_elap_frames++;
	}
	if (is_aarp_frame(buffer, length)) {
		ESP_LOGI(TAG, "got AARP frame");
		stats.eth_recv_aarp_frames++;
	}
	
	if (is_appletalk_frame(buffer, length) || is_aarp_frame(buffer, length)) {
		// We intercept appletalk and aarp frames
		if (ethernet_transport_enabled) {
			buffer_t *buff = wrapbuf(buffer, length);
			BaseType_t err = xQueueSendToBack(ethertalkv2_inbound_queue,
				&buff, (TickType_t)0);
				
			if (err != pdTRUE) {
				stats.transport_in_errors__transport_ethernet__err_lap_queue_full++;
				freebuf(buff);
			}
			
			return ESP_OK;
		} else {
			// We have no ethernet transport, quietly drop buffer on the floor
			free(buffer);
			return ESP_OK;
		}
	} else {

	// if we intercept buffer, we have to free it

    return esp_netif_receive((esp_netif_t *)priv, buffer, length, NULL);
    }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
	ESP_LOGI(TAG, "eth got ip, kicking off dependent tasks");
	ip_ready = true;
}

void start_ethernet(void) {
	/* set up ESP32 internal MAC */
	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	mac_config.sw_reset_timeout_ms = 1000;

	eth_esp32_emac_config_t emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
	emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
	emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;
	emac_config.smi_gpio.mdc_num = ETH_MAC_MDC;
	emac_config.smi_gpio.mdio_num = ETH_MAC_MDIO;
	
	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_config, &mac_config);

	/* set up PHY */
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = 1;
	phy_config.reset_gpio_num = -1;
	esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

	// Enable external oscillator (pulled down at boot to allow IO0 strapping)
	ESP_ERROR_CHECK(gpio_set_direction(ETH_50MHZ_EN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(ETH_50MHZ_EN, 1));
	
	ESP_LOGD(TAG, "Starting Ethernet interface...");

	// Install and start Ethernet driver
	esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
	esp_eth_handle_t eth_handle = NULL;
	ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

	esp_netif_config_t const netif_config = ESP_NETIF_DEFAULT_ETH();
	esp_netif_t *global_netif = esp_netif_new(&netif_config);
	esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
	ESP_ERROR_CHECK(esp_netif_attach(global_netif, eth_netif_glue));
	char* hostname = generate_hostname();
	ESP_ERROR_CHECK(esp_netif_set_hostname(global_netif, hostname));
	free(hostname);
	
	// Install our custom input and output path
	ESP_ERROR_CHECK(esp_eth_update_input_path(eth_handle, ethernet_input_path, global_netif));
	munge_ethernet_output_path(eth_handle, global_netif);
	
	// register handler for when we get an IP
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
	
	active_ip_net_if = eth_handle;
	
	ethertalkv2_inbound_queue = xQueueCreate(ETHERNET_QUEUE_DEPTH, sizeof(buffer_t*));
	ethertalkv2_outbound_queue = xQueueCreate(ETHERNET_QUEUE_DEPTH, sizeof(buffer_t*));
}

esp_err_t ethertalkv2_transport_enable(transport_t* dummy) {
	ethernet_transport_enabled = true;
	return ESP_OK;
}

esp_err_t ethertalkv2_transport_disable(transport_t* dummy) {
	ethernet_transport_enabled = false;
	return ESP_OK;
}

static transport_t ethertalkv2_transport = {
	.kind = "ethertalk_v2",
	.private_data = NULL,
	
	.enable = &ethertalkv2_transport_enable,
	.disable = &ethertalkv2_transport_disable,
};

transport_t* ethertalkv2_get_transport(void) {
	// warn if we do something silly and attempt to get multiple
	// transports for this interface
	static int attempts = 0;
	if (attempts > 0) {
		ESP_LOGE(TAG, "multiple transports requested for ethertalkv2; beware - there can be only one!");
	}
	attempts++;
	
	ethertalkv2_transport.inbound = ethertalkv2_inbound_queue;
	ethertalkv2_transport.outbound = ethertalkv2_outbound_queue;
	
	return &ethertalkv2_transport;
}