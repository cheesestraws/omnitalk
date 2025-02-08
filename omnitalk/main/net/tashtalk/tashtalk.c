#include "net/tashtalk/tashtalk.h"

#include <esp_log.h>

#include "net/transport.h"
#include "net/tashtalk/uart.h"

static const char* TAG="TASHTALK_TRANSPORT";

void start_tashtalk(void) {
	tt_uart_init();
	tt_uart_start();
}

esp_err_t tashtalk_transport_enable(transport_t* dummy) {
	rxstate->send_output_to_queue = true;
	return ESP_OK;
}

esp_err_t tashtalk_transport_disable(transport_t* dummy) {
	rxstate->send_output_to_queue = false;
	return ESP_OK;
}

static transport_t tashtalk_transport = {
	.kind = "tashtalk",
	.private_data = NULL,
	
	.enable = &tashtalk_transport_enable,
	.disable = &tashtalk_transport_disable,
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