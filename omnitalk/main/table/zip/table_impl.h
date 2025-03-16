#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct zip_zone_node_s {
	bool dummy;
	
	char* zone_name;
	struct zip_zone_node_s* next;	
};

struct zip_network_node_s {
	bool dummy;

	uint16_t net_start;
	uint16_t net_end;
	
	int zone_count;
	int expected_zone_count;
	bool complete;
	
	struct zip_zone_node_s root;
	
	struct zip_network_node_s* next;
};

typedef struct {
	SemaphoreHandle_t mutex;
	
	struct zip_network_node_s root;
} zt_zip_table_t;

