#include "net/transport.h"

#include <esp_err.h>

#include "mem/buffers.h"

esp_err_t enable_transport(transport_t* transport) {
	return transport->enable(transport);
}

esp_err_t disable_transport(transport_t* transport) {
	return transport->disable(transport);
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
		buff, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}

bool tsend_and_block(transport_t* transport, buffer_t *buff) {
	BaseType_t err = xQueueSendToBack(transport->outbound,
		buff, portMAX_DELAY);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}