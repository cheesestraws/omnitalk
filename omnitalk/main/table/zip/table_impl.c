#include "table/zip/table.h"
#include "table/zip/table_impl.h"

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

zt_zip_table_t* zt_new() {
	zt_zip_table_t* table = calloc(1, sizeof(zt_zip_table_t));
	
	table->mutex = xSemaphoreCreateMutex();
	table->root.dummy = true;
	table->root.root.dummy = true;
	
	return table;
}

size_t zt_count_net_ranges(zt_zip_table_t *table) {
	size_t count = 0;
	struct zip_network_node_s* curr;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	for (curr = &table->root; curr != NULL; curr = curr->next) {
		count++;
	}
	
	xSemaphoreGive(table->mutex);
	return count;
}

bool zt_add_net_range_unguarded(zt_zip_table_t *table, uint16_t net_start, uint16_t net_end) {
	struct zip_network_node_s* curr;
	struct zip_network_node_s* prev = NULL;
	
	for (curr = &table->root; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		// if we've found the range, cool, bail out
		if (curr->net_start == net_start && curr->net_end == net_end) {
			return false;
		}
		
		// if not, and we've got past the start network number, add it here
		if (curr->net_start > net_start) {
			break;
		}
	}
	
	struct zip_network_node_s* new_node = calloc(1, sizeof(struct zip_network_node_s));
	new_node->net_start = net_start;
	new_node->net_end = net_end;
	new_node->root.dummy = true;
	
	new_node->next = curr;
	prev->next = new_node;
	
	return true;
}

bool zt_add_net_range(zt_zip_table_t *table, uint16_t net_start, uint16_t net_end) {
	bool result;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	result = zt_add_net_range_unguarded(table, net_start, net_end);
	
	xSemaphoreGive(table->mutex);

	return result;
}
