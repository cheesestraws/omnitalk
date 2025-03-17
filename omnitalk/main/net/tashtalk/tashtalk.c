#include "net/tashtalk/tashtalk.h"

#include <esp_log.h>

#include "net/transport.h"
#include "net/tashtalk/uart.h"
#include "tunables.h"

static const char* TAG="TASHTALK_TRANSPORT";

esp_err_t tashtalk_transport_enable(transport_t* dummy) {
	tashtalk_enable_uart_tx = true;
	rxstate->send_output_to_queue = true;
	return ESP_OK;
}

esp_err_t tashtalk_transport_disable(transport_t* dummy) {
	tashtalk_enable_uart_tx = false;
	rxstate->send_output_to_queue = false;
	return ESP_OK;
}

static esp_err_t tashtalk_set_node_address(transport_t* dummy, uint8_t addr) {
	ESP_LOGI(TAG, "setting address to %d", (int)addr);
	tashtalk_node_address = addr;
	return tt_uart_refresh_address();
}

transport_t tashtalk_transport = {
	.quality = QUALITY_LOCALTALK, // LocalTalk is the lowest bandwidth local 

	.kind = "tashtalk",
	.private_data = NULL,
	
	.enable = &tashtalk_transport_enable,
	.disable = &tashtalk_transport_disable,
	.set_node_address = &tashtalk_set_node_address,
};

transport_t* tashtalk_get_transport(void) {
	// warn if we do something silly and attempt to get multiple
	// transports for this interface
	static int attempts = 0;
	if (attempts > 0) {
		ESP_LOGE(TAG, "multiple transports requested for tashtalk; beware - there can be only one!");
	}
	attempts++;
	
	tashtalk_transport.inbound = tashtalk_inbound_queue;
	tashtalk_transport.outbound = tashtalk_outbound_queue;

	return &tashtalk_transport;	
}

void start_tashtalk(void) {
	tashtalk_transport.ready_event = xEventGroupCreate();

	tt_uart_init();
	tt_uart_start();
}
