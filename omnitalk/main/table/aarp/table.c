#include "table/aarp/table.h"

#include <stdbool.h>
#include <stddef.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

#include "tunables.h"

aarp_table_t* aarp_new_table() {
	aarp_table_t* table = calloc(1, sizeof(aarp_table_t));
	
	table->mutex = xSemaphoreCreateMutex();

	return table;
}

// not static so we can call it from tests
size_t count_list_entries_unguarded(struct aarp_node_s *list) {
	size_t count = 0;
	for (struct aarp_node_s *curr = list; curr != NULL; curr = curr->next) {
		if (curr->valid) {
			count++;
		}
	}
	return count;
}

size_t aarp_table_entry_count(aarp_table_t* table) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	size_t count = 0;
	
	for (int i = 0; i < AARP_TABLE_BUCKETS; i++) {
		count += count_list_entries_unguarded(&table->buckets[i]);
	}
	
	xSemaphoreGive(table->mutex);
	return count;
}

static void touch_in_list_unguarded(struct aarp_node_s *list, uint16_t network, uint8_t node, struct eth_addr hwaddr) {
	bool found = false;
	
	struct aarp_node_s *prev = NULL;
	for (struct aarp_node_s *curr = list; curr != NULL; prev = curr, curr = curr->next) {
		if (!curr->valid) {
			continue;
		}
		
		if (curr->network == network && curr->node == node) {
			curr->hwaddr = hwaddr;
			curr->timestamp = esp_timer_get_time();
			
			found = true;
		}
	}
	
	if (!found) {
		// We didn't have a matching AARP entry.  Let's create one.
		struct aarp_node_s *new_node = calloc(1, sizeof(struct aarp_node_s));
		
		new_node->valid = true;
		new_node->network = network;
		new_node->node = node;
		new_node->hwaddr = hwaddr;
		
		new_node->timestamp = esp_timer_get_time();
		
		// prev contains the tail of the list at this point
		prev->next = new_node;
	}
}

static void aarp_touch_unguarded(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr hwaddr) {
	// First, we need to find the bucket this should go in.  We only use the node
	// address to calculate this until I get a better idea because most people
	// won't have really large network ranges with highly duplicated addresses.
	
	int bucket = node % AARP_TABLE_BUCKETS;
	
	touch_in_list_unguarded(&table->buckets[bucket], network, node, hwaddr);
}

void aarp_touch(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr hwaddr) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	aarp_touch_unguarded(table, network, node, hwaddr);
	xSemaphoreGive(table->mutex);
}

static bool aarp_lookup_in_list_unguarded(struct aarp_node_s *list, uint16_t network, uint8_t node, struct eth_addr* out) {
	bool found = false;

	for (struct aarp_node_s *curr = list; curr != NULL; curr = curr->next) {
		if (!curr->valid) {
			continue;
		}
		
		if (curr->network == network && curr->node == node) {
			*out = curr->hwaddr;
			found = true;
		}
	}
	
	return found;

	return false;
}

static bool aarp_lookup_unguarded(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr* out) {
	int bucket = node % AARP_TABLE_BUCKETS;

	return aarp_lookup_in_list_unguarded(&table->buckets[bucket], network, node, out);
}

bool aarp_lookup(aarp_table_t* table, uint16_t network, uint8_t node, struct eth_addr* out) {
	bool result;

	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	result = aarp_lookup_unguarded(table, network, node, out);
	xSemaphoreGive(table->mutex);
	
	return result;
}
