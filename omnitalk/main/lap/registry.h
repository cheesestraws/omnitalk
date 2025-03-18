#pragma once

#include <stdatomic.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

#include "lap/lap_types.h"

// The LAP registry keeps track of what LAPs there are and whether they're ready or
// not.  It allows efficiently waiting for all LAPs to be ready, and also retrieval
// of the highest quality LAP we have.

struct lap_registry_node_s {
	bool dummy;
	lap_t *lap;
	
	struct lap_registry_node_s *next;
};

typedef struct lap_registry_s {
	EventGroupHandle_t ready_laps;
		
	SemaphoreHandle_t mutex;
	EventBits_t registered_laps;
	struct lap_registry_node_s root;
} lap_registry_t;

lap_registry_t* lap_registry_new();
int lap_registry_lap_count(lap_registry_t *registry);
void lap_registry_register(lap_registry_t* registry, lap_t *lap);
lap_t* lap_registry_highest_quality_lap(lap_registry_t* registry);
