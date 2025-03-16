#include "table/zip/table.h"
#include "table/zip/table_impl.h"

#include <stdbool.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "util/string.h"

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
	struct zip_zone_node_s *to_be_deleted = NULL;
	while (curr != NULL) {
		to_be_deleted = curr;
		curr = curr->next;
		
		free(to_be_deleted->zone_name);
		free(to_be_deleted);
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

static void zt_add_zone_for_unguarded(zt_zip_table_t *table, uint16_t network, char* zone) {
	struct zip_network_node_s* net_node = zt_lookup_unguarded(table, network);
	if (net_node == NULL) {
		return;
	}
	
	struct zip_zone_node_s *prev = NULL;
	struct zip_zone_node_s *curr;
	// Iterate through the zonelist to see if we've found it
	for(curr = &net_node->root; curr != NULL; prev = curr, curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
	
		int cmp = strcmp(curr->zone_name, zone);
		
		// Is this zone name already in the zonelist?
		if (cmp == 0) {
			// Yes, return, leaving the zonelist untouched
			return;
		}
		
		if (cmp > 0) {
			// We've reached a string later in lexical order than the zone name;
			// we know it's not going to be here now.
			break;
		}
	}
	
	// This should be impossible, but let's not bugger the heap if prev is unset
	if (prev == NULL) { 
		return;
	}
	
	// If we reach here, we didn't find the zone, and prev will point to where we
	// ought to put it.
	char* new_zone_name = NULL;
	struct zip_zone_node_s *new_node = calloc(1, sizeof(*new_node));
	if (new_node == NULL) {
		goto cleanup;
	}
	
	new_zone_name = strclone(zone);
	if (new_zone_name == NULL) {
		goto cleanup;
	}
	
	new_node->zone_name = new_zone_name;
	new_node->next = curr;
	prev->next = new_node;
	
	return;
	
cleanup:
	if (new_node != NULL) {
		free(new_node);
	}
	
	if (new_zone_name != NULL) {
		free(new_zone_name);
	}
}


void zt_add_zone_for(zt_zip_table_t *table, uint16_t network, char* zone) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	zt_add_zone_for_unguarded(table, network, zone);
	
	xSemaphoreGive(table->mutex);
}

static void zt_mark_network_complete_unguarded(zt_zip_table_t *table, uint16_t network) {
	// find node for network
	struct zip_network_node_s* net_node = zt_lookup_unguarded(table, network);
	if (net_node == NULL) {
		return;
	}

	net_node->complete = true;
}

void zt_mark_network_complete(zt_zip_table_t *table, uint16_t network) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	zt_mark_network_complete_unguarded(table, network);
	xSemaphoreGive(table->mutex);
}

size_t zt_count_zones_for(zt_zip_table_t *table, uint16_t network) {
	size_t count = 0;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	
	struct zip_network_node_s* net_node = zt_lookup_unguarded(table, network);
	if (net_node == NULL) {
		return 0;
	}
	
	struct zip_zone_node_s *curr;
	for(curr = &net_node->root; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		
		count++;
	}
	
	xSemaphoreGive(table->mutex);
	
	return count;
}

bool zt_network_is_complete(zt_zip_table_t *table, uint16_t network) {
	bool result;
	
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	struct zip_network_node_s* net_node = zt_lookup_unguarded(table, network);
	if (net_node == NULL) {
		result = false;
	} else {
		result = net_node->complete;
	}

	xSemaphoreGive(table->mutex);
	
	return result;
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
