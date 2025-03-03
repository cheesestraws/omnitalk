#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <lwip/prot/ethernet.h>

#include "tunables.h"

struct aarp_node_s {
	// Inserting a dummy node at the start of the list because it makes deleting easier.
	// dummy nodes have valid=0, so we don't need to init loads of dummies
	bool valid;
	
	uint16_t network;
	uint8_t node;
	struct eth_addr hwaddr;
	
	int64_t timestamp;
	
	struct aarp_node_s *next;
};


typedef struct {
	SemaphoreHandle_t mutex;
	
	struct aarp_node_s buckets[AARP_TABLE_BUCKETS];
} aarp_table_t;


aarp_table_t* aarp_new_table();
size_t aarp_table_entry_count(aarp_table_t* table);
void aarp_touch(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr hwaddr);
bool aarp_lookup(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr* out);
