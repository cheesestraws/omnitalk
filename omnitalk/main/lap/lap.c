#include "lap.h"

#include "net/transport.h"
#include "web/stats.h"

lsend_mock_t lap_lsend_mock;

bool lsend(lap_t *lap, buffer_t *buff) {
	if (lap_lsend_mock != NULL) {
		return lap_lsend_mock(lap, buff);
	}

	BaseType_t err = xQueueSendToBack(lap->outbound,
		&buff, 0);
	
	if (err != pdTRUE) {
		return false;
	} 
	
	return true;
}

void lap_set_my_zone(lap_t *lap, pstring* zone) {
	lap->my_zone = zone;
	stats_lap_metadata[lap->id].zone = pstring_to_cstring_alloc(zone);
}

bool lap_supports_ether_multicast(lap_t *lap) {
	return transport_supports_ether_multicast(lap->transport);
}

struct eth_addr lap_ether_multicast_for_zone(lap_t *lap, pstring *zone) {
	return transport_ether_multicast_for_zone(lap->transport, zone);
}
