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

static struct zip_network_node_s* zt_lookup_unguarded(zt_zip_table_t *table, uint16_t network) {
	struct zip_network_node_s* curr;

	for (curr = &table->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
	
		if (curr->net_start <= network && curr->net_end >= network) {
			return curr;
		}
	}
	
	return NULL;
}

size_t zt_count_net_ranges(zt_zip_table_t *table) {
	size_t count = 0;
	struct zip_network_node_s* curr;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	for (curr = &table->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		count++;
	}
	
	xSemaphoreGive(table->mutex);
	return count;
}

bool zt_contains_net(zt_zip_table_t *table, uint16_t network) {
	bool found = false;
	struct zip_network_node_s* curr;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	for (curr = &table->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
	
		if (curr->net_start <= network && curr->net_end >= network) {
			found = true;
			break;
		}
		if (curr->net_start > network) {
			break;
		}
	}
	
	xSemaphoreGive(table->mutex);
	return found;
}

static bool zt_add_net_range_unguarded(zt_zip_table_t *table, uint16_t net_start, uint16_t net_end) {
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

static void free_all_zones_for(struct zip_network_node_s* n) {
	struct zip_zone_node_s *curr = n->root.next;
	while (curr != NULL) {
		assert(false); // FIX THIS LATER
	}
}

static bool zt_delete_network_unguarded(zt_zip_table_t *table, uint16_t network) {
	bool deleted = false;
	struct zip_network_node_s* curr;
	struct zip_network_node_s* prev = NULL;

	for (curr = &table->root; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		if (curr->net_start <= network && curr->net_end >= network) {
			free_all_zones_for(curr);
			prev->next = curr->next;
			free(curr);
			deleted = true;
		}

		if (curr->net_start > network) {
			break;
		}
	}
	
	return deleted;
}

bool zt_delete_network(zt_zip_table_t *table, uint16_t network) {
	bool result;
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	result = zt_delete_network_unguarded(table, network);
	
	xSemaphoreGive(table->mutex);
	return result;

}

bool zt_get_network_complete(zt_zip_table_t *table, uint16_t network) {
	struct zip_network_node_s *node;
	bool complete;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	node = zt_lookup_unguarded(table, network);
	if (node == NULL) {
		// We return true if the network isn't in the table; this is really
		// going to be a bug, but if we return false here, we'll generate unnecessary
		// ZIP traffic in the presence of such a bug, which might effectively DoS
		// the network.  Let's not do that.  It's rude.
		complete = true;
	} else {
		complete = node->complete;
	}
	
	
	xSemaphoreGive(table->mutex);
	return complete;	
}

void zt_print(zt_zip_table_t *table) {
	struct zip_network_node_s* curr;

	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	for (curr = &table->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		printf("net %d - %d\n", curr->net_start, curr->net_end);
	}

	
	xSemaphoreGive(table->mutex);
}
