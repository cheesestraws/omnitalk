#include "net/transport.h"

#include <esp_err.h>

esp_err_t enable_transport(transport_t* transport) {
	return transport->enable(transport);
}

esp_err_t disable_transport(transport_t* transport) {
	return transport->disable(transport);
}