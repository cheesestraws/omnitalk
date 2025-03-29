#include "net/transport.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_err.h>

#include "mem/buffers.h"

esp_err_t enable_transport(transport_t* transport) {
	return transport->enable(transport);
}

esp_err_t disable_transport(transport_t* transport) {
	return transport->disable(transport);
}

void wait_for_transport_ready(transport_t* transport) {
	xEventGroupWaitBits(
		transport->ready_event,
		1,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY
	);
}

void mark_transport_ready(transport_t* transport) {
	xEventGroupSetBits(transport->ready_event, 1);
}

esp_err_t set_transport_node_address(transport_t* transport, uint8_t node_address) {
	if (transport->set_node_address != NULL) {
		return transport->set_node_address(transport, node_address);
	}
	return ESP_OK;
}

bool transport_supports_ether_multicast(transport_t* transport) {
	return transport->get_zone_ether_multicast != NULL;
}

struct eth_addr transport_ether_multicast_for_zone(transport_t* transport, pstring* zone) {
	return transport->get_zone_ether_multicast(transport, zone);
}


buffer_t* trecv(transport_t* transport) {
	buffer_t *buff = NULL;
	xQueueReceive(transport->inbound, &buff, portMAX_DELAY);
	return buff;
}

buffer_t* trecv_with_timeout(transport_t* transport, TickType_t timeout) {
	buffer_t *buff = NULL;
	xQueueReceive(transport->inbound, &buff, timeout);
	return buff;
}


bool tsend(transport_t* transport, buffer_t *buff) {
	BaseType_t err = xQueueSendToBack(transport->outbound,
		&buff, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}

bool tsend_and_block(transport_t* transport, buffer_t *buff) {
	BaseType_t err = xQueueSendToBack(transport->outbound,
		&buff, portMAX_DELAY);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}
