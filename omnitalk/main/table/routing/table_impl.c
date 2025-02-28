#include "table/routing/table.h"
#include "table/routing/table_impl.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "table/routing/route.h"

#define MICROSECONDS 1000000

rt_routing_table_t* rt_new() {
	rt_routing_table_t* table = calloc(1, sizeof(rt_routing_table_t));
	
	table->mutex = xSemaphoreCreateMutex();
	table->list.dummy = true;
	
	return table;
}


static void rt_touch_unguarded(rt_routing_table_t* table, rt_route_t r) {
	bool found = false;
	
	// Start at the top of the list
	struct rt_node_s *prev = NULL;
	struct rt_node_s *curr = &table->list;
		
	while (curr != NULL) {
		// skip dummy entries
		if (curr->dummy) {
			goto next_item;
		}
		
		// if we've already passed our route's distance, bail out
		if (curr->route.distance > r.distance) {
			break;
		}
		
		if (rt_routes_equal(&curr->route, &r)) {
			found = true;
			curr->last_touched_timestamp = esp_timer_get_time();
			break;
		}
		
		// If the routes aren't equal, but they match, then the distance
		// has changed: we need to remove the old route and free it and
		// replace it with the new one.
		if (rt_routes_match(&curr->route, &r)) {
			struct rt_node_s *new_curr = curr->next;
			prev->next = curr->next;
			free(curr);
			curr = new_curr;
			
			// Don't fall through or we will skip new_curr; re-loop
			// with the same 'prev'.
			continue;
		}
	
	next_item:
		prev = curr;
		curr = curr->next;
	}
	
	// Did we find and update it?
	if (found) {
		return;
	}
	
	// No, so construct a new node.
	struct rt_node_s *new_node = calloc(1, sizeof(struct rt_node_s));
	new_node->dummy = false;
	memcpy(&new_node->route, &r, sizeof(rt_route_t));
	new_node->last_touched_timestamp = esp_timer_get_time();
	
	// and insert it into the list
	new_node->next = curr;
	prev->next = new_node;
}

void rt_touch(rt_routing_table_t* table, rt_route_t r) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	rt_touch_unguarded(table, r);
	xSemaphoreGive(table->mutex);
}

static bool rt_lookup_unguarded(rt_routing_table_t* table, uint16_t network_number, rt_route_t *out) {
	int64_t now = esp_timer_get_time();

	for (struct rt_node_s *curr = &table->list; curr != NULL; curr = curr->next) {
		// skip dummy node
		if (curr->dummy) {
			continue;
		}
		
		if (network_number >= curr->route.range_start && network_number <= curr->route.range_end) {
			// Is the route fresh enough?  Note that esp_timer_get_time returns a signed
			// result so we don't need to worry about being up for less than a minute here
			if (curr->last_touched_timestamp < now - (NODE_DEAD_THRESHOLD_SECS * MICROSECONDS)) {
				continue;
			}
		
			memcpy(out, &curr->route, sizeof(rt_route_t));
			return true;
		}
	}
	
	return false;
}


bool rt_lookup(rt_routing_table_t* table, uint16_t network_number, rt_route_t *out) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	bool result = rt_lookup_unguarded(table, network_number, out);
	
	xSemaphoreGive(table->mutex);
	
	return result;
}

static void rt_route_print(rt_route_t *a) {
	printf("    net range %d - %d dist %d via %d.%d (%s -> %s) ",
		(int)a->range_start, (int)a->range_end,
		(int)a->distance,
		(int)a->nexthop.network, (int)a->nexthop.node,
		(a->outbound_lap != NULL ? a->outbound_lap->name : "NULL"),
		(a->outbound_lap != NULL && a->outbound_lap->transport != NULL ? a->outbound_lap->transport->kind : "NULL"));
}

void rt_print(rt_routing_table_t* table) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	int64_t now = esp_timer_get_time();

	printf("routing table:\n");
	
	for (struct rt_node_s *curr = &table->list; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}

		rt_route_print(&curr->route);
		
		if (curr->last_touched_timestamp < now - (NODE_DEAD_THRESHOLD_SECS * MICROSECONDS)) {
			printf(" (dead)");
		} else if (curr->last_touched_timestamp < now - (NODE_BAD_THRESHOLD_SECS * MICROSECONDS)) {
			printf(" (bad)");
		} else {
			printf(" (ok)");
		}
		
		int64_t elapsed_millis = (now - curr->last_touched_timestamp) / 1000;
		printf(" [%" PRId64 "ms ago]\n", elapsed_millis);
	}

	xSemaphoreGive(table->mutex);
}
