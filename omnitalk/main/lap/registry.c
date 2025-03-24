#include "lap/registry.h"

#include <stdatomic.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include "web/stats.h"

lap_registry_t* lap_registry_new() {
	lap_registry_t *reg = calloc(1, sizeof(*reg));
	reg->ready_laps = xEventGroupCreate();
	reg->mutex = xSemaphoreCreateMutex();
	reg->root.dummy = true;
	
	return reg;
}

static int lap_registry_lap_count_unguarded(lap_registry_t* registry) {
	int count = 0;
	
	struct lap_registry_node_s *curr = NULL;
	for (curr = &registry->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		count++;
	}
	
	return count;
}

int lap_registry_lap_count(lap_registry_t *registry) {
	int count;
	while (xSemaphoreTake(registry->mutex, portMAX_DELAY) != pdTRUE) {}
	count = lap_registry_lap_count_unguarded(registry);
	xSemaphoreGive(registry->mutex);

	return count;
}

void lap_registry_register(lap_registry_t* registry, lap_t *lap) {
	while (xSemaphoreTake(registry->mutex, portMAX_DELAY) != pdTRUE) {}
	
	// Add this LAP to the bitfield
	registry->registered_laps = registry->registered_laps | (1 << lap->id);
	
	bool lap_exists = false;
	
	// And to the list, sorting by LAP quality
	struct lap_registry_node_s *prev = NULL;
	struct lap_registry_node_s *curr = NULL;
	
	for (curr = &registry->root; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		if (curr->lap == lap) {
			lap_exists = true;
		}
		
		if (curr->lap->quality < lap->quality) {
			break;
		}
	}
	
	if (!lap_exists) {
		struct lap_registry_node_s *new_node;
		new_node = calloc(1, sizeof(*new_node));
		new_node->lap = lap;
		new_node->next = curr;
		prev->next = new_node;
	}
	
	stats.lap_registry_registered_laps = lap_registry_lap_count_unguarded(registry);
	
	xSemaphoreGive(registry->mutex);
}

lap_t* lap_registry_highest_quality_lap(lap_registry_t* registry) {
	lap_t *lap = NULL;

	while (xSemaphoreTake(registry->mutex, portMAX_DELAY) != pdTRUE) {}
	
	struct lap_registry_node_s *first = registry->root.next;
	if (first != NULL) {
		lap = first->lap;
	}
	
	xSemaphoreGive(registry->mutex);
	
	return lap;
}

void lap_registry_update_zone_cache(lap_registry_t *registry) {
	static bool previously_set_stats_zone = false;

	while (xSemaphoreTake(registry->mutex, portMAX_DELAY) != pdTRUE) {}

	pstring* zone = NULL;	
	struct lap_registry_node_s *curr = NULL;
	
	for (curr = &registry->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		if (curr->lap != NULL && curr->lap->my_zone != NULL) {
			zone = curr->lap->my_zone;
			break;
		}
	}
	
	if (zone != NULL) {
		registry->best_zone_cache = zone;
		char* old_stats_zone = atomic_exchange(&stats_omnitalk_metadata.best_zone, pstring_to_cstring_alloc(zone));
		if (previously_set_stats_zone && old_stats_zone != NULL) {
			free(old_stats_zone);
		}
		previously_set_stats_zone = true;

	}
	
	xSemaphoreGive(registry->mutex);
}

bool lap_registry_get_best_address(lap_registry_t *registry, uint16_t *out_net, uint8_t *out_node) {
	while (xSemaphoreTake(registry->mutex, portMAX_DELAY) != pdTRUE) {}
	bool found = false;

	struct lap_registry_node_s *curr = NULL;
	
	for (curr = &registry->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		if (curr->lap != NULL && curr->lap->my_address != 0 && curr->lap->my_network != 0) {
			found = true;
			*out_net = curr->lap->my_network;
			*out_node = curr->lap->my_address;
			break;
		}
	}
	
	xSemaphoreGive(registry->mutex);
	return found;
}
