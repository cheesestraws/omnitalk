#include "table/routing/table.h"
#include "table/routing/table_impl.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "table/routing/route.h"
#include "util/event/event.h"

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
				
		if (rt_routes_equal(&curr->route, &r)) {
			found = true;
			curr->last_touched_timestamp = esp_timer_get_time();
			if (r.distance != 31) {
				curr->status = RT_GOOD;
			}
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
	
	// Direct routes have distance = 0
	if (r.distance == 0) {
		new_node->status = RT_DIRECT;
	} else if (r.distance == 31) {
		// 31 is "known BAD route"
		new_node->status = RT_BAD;
	} else {
		new_node->status = RT_GOOD;
	}
	
	// and insert it into the list in the right position
	prev = NULL;
	curr = &table->list;
		
	while (curr != NULL) {
		// skip dummy entries
		if (curr->dummy) {
			goto next_item_ins;
		}

		// Is this beyond our new distance?
		if (curr->route.distance > new_node->route.distance) {
			break;
		}
		
	next_item_ins:
		prev = curr;
		curr = curr->next;
	}

	new_node->next = curr;
	prev->next = new_node;
	
	event_fire(&table->touch_event, &r);
}

void rt_touch(rt_routing_table_t* table, rt_route_t r) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	rt_touch_unguarded(table, r);
	xSemaphoreGive(table->mutex);
}

void rt_touch_direct(rt_routing_table_t* table, uint16_t start, uint16_t end, lap_t *lap) {
	rt_route_t route = {
		.range_start = start,
		.range_end = end,

		.outbound_lap = lap,
		.nexthop = { 0 },
		.distance = 0,
	};
	
	rt_touch(table, route);	
}

static bool rt_lookup_unguarded(rt_routing_table_t* table, uint16_t network_number, rt_route_t *out) {
	for (struct rt_node_s *curr = &table->list; curr != NULL; curr = curr->next) {
		// skip dummy node
		if (curr->dummy) {
			continue;
		}
		
		if (network_number >= curr->route.range_start && network_number <= curr->route.range_end) {
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

static void rt_prune_unguarded(rt_routing_table_t* table) {
	// The "bad list" is the list of routes that have been downgraded to "bad".
	// Their distance is downgraded to 31, and they're pushed to the bottom of the
	// candidate list.
	struct rt_node_s *bad_node_list_head = NULL;
	struct rt_node_s *bad_node_list_tail = NULL;
	
	// The "deleted list" is the list of routes that will be deleted from the routing
	// table.
	struct rt_node_s *deleted_node_list_head = NULL;
	struct rt_node_s *deleted_node_list_tail = NULL;
	
	rt_route_t scratch_route;
	rt_route_t deleted_route;

	// Start at the top of the list
	struct rt_node_s *prev = NULL;
	struct rt_node_s *curr = &table->list;
	
	// Filter out dead routes
	struct rt_node_s *new_curr;
	while (curr != NULL) {
		// skip dummy entries
		if (curr->dummy) {
			goto next_item;
		}
		
		bool removed = false;
		
		switch (curr->status) {
		case RT_DIRECT:
			// We never age out direct routes.
			break;
		case RT_SUSPECT:
			// Routes that stay suspect get downgraded to bad
			curr->status = RT_BAD;
			curr->route.distance = 31;
			
			struct rt_node_s *to_be_demoted = curr;
			
			// Remove it from the main routing table but DON'T free it
			new_curr = curr->next;
			prev->next = curr->next;
			curr = new_curr;
			removed = true;
			
			// And instead add it to the bad route list
			to_be_demoted->next = NULL;
			if (bad_node_list_head == NULL) {
				bad_node_list_head = to_be_demoted;
				bad_node_list_tail = to_be_demoted;
			} else {
				bad_node_list_tail->next = to_be_demoted;
				bad_node_list_tail = to_be_demoted;
			}
			
			break;
		case RT_GOOD:
			curr->status = RT_SUSPECT;
			break;
		case RT_BAD:
			struct rt_node_s *to_be_deleted = curr;
		
			new_curr = curr->next;
			prev->next = curr->next;
			curr = new_curr;
			removed = true;
			
			if (deleted_node_list_head == NULL) {
				deleted_node_list_head = to_be_deleted;
				deleted_node_list_tail = to_be_deleted;
			} else {
				deleted_node_list_tail->next = to_be_deleted;
				deleted_node_list_tail = to_be_deleted;
			}

			break;
		}
				
		if (removed) {
			// if we removed a node, we don't want to increase 'curr'.
			continue;
		}

	next_item:
		prev = curr;
		curr = curr->next;
	}
	
	// Now we graft on the bad routes at the end
	// prev will be the list tail
	prev->next = bad_node_list_head;
	
	// Then iterate through the dead routes to see if we need to fire any delete
	// events
	curr = deleted_node_list_head;
	while (curr != NULL) {
		// if we deleted a route, we should check whether we have any other
		// routes for that network range; if not, we should fire a 'network range
		// deleted' event.  This is slow and we need to make it faster.
		deleted_route = curr->route;
		bool other_route_exists = rt_lookup_unguarded(table, deleted_route.range_start, &scratch_route);
		if (!other_route_exists) {
			event_fire(&table->network_range_deleted_event, &deleted_route);
		}
		
		prev = curr;
		curr = curr->next;
		free(prev);
	}
}

void rt_prune(rt_routing_table_t* table) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}
	rt_prune_unguarded(table);
	xSemaphoreGive(table->mutex);
}

static char* rt_route_lap_name(rt_route_t *a) {
	return a->outbound_lap != NULL && a->outbound_lap->name != NULL ? a->outbound_lap->name : "NULL";
}

static char* rt_route_lap_kind(rt_route_t *a) {
	return a->outbound_lap != NULL && a->outbound_lap->kind != NULL ? a->outbound_lap->kind : "NULL";
}

static char* rt_route_transport_name(rt_route_t *a) {
	return a->outbound_lap != NULL && a->outbound_lap->transport != NULL && a->outbound_lap->transport->kind != NULL ? a->outbound_lap->transport->kind : "NULL";
}

void rt_route_print(rt_route_t *a) {
	printf("    net range %d - %d dist %d via %d.%d (%s -> %s) ",
		(int)a->range_start, (int)a->range_end,
		(int)a->distance,
		(int)a->nexthop.network, (int)a->nexthop.node,
		rt_route_lap_name(a),
		rt_route_transport_name(a));
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
		
		switch (curr->status) {
		case RT_DIRECT:
			printf(" (direct)"); break;
		case RT_SUSPECT:
			printf(" (suspect)"); break;
		case RT_GOOD:
			printf(" (good)"); break;
		case RT_BAD:
			printf(" (bad)"); break;
		}
		
		int64_t elapsed_millis = (now - curr->last_touched_timestamp) / 1000;
		printf(" [%" PRId64 "ms ago]\n", elapsed_millis);
	}

	xSemaphoreGive(table->mutex);
}

char* rt_stats(rt_routing_table_t* table) {
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	// first, count the number of nodes (and while we're doing it,
	// work out what's the longest LAP and transport names
	struct rt_node_s *curr;
	int node_count = 0;
	int longest_lap_name = 0;
	int longest_lap_kind = 0;
	int longest_transport_name = 0;
	
	for (curr = &table->list; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		node_count++;
		
		int lap_len = strlen(rt_route_lap_name(&curr->route));
		int lap_kind_len = strlen(rt_route_lap_kind(&curr->route));
		int transport_len = strlen(rt_route_transport_name(&curr->route));
		
		if (lap_len > longest_lap_name) { longest_lap_name = lap_len; }
		if (lap_kind_len > longest_lap_kind) { longest_lap_kind = lap_kind_len; }
		if (transport_len > longest_transport_name) { longest_transport_name = transport_len; }
	}

	// Now, work out how much memory each route will take
	char* fmt = "route{address_family=\"atalk\", "
		"net_range_start=\"%" PRIu16 "\", net_range_end=\"%" PRIu16 "\", "
		"lap=\"%s\", lap_kind=\"%s\", transport=\"%s\", "
		"nexthop=\"%" PRIu16 ".%" PRIu8 "\", distance=\"%" PRIu8 "\", "
		"status=\"%s\""
		"} 1\n";
	
	int route_string_length = strlen(fmt);
	
	// As a pessimistic estimate, we assume that each byte might represent
	// three bytes of ASCII digits
	route_string_length += sizeof(rt_route_t) * 3;
	
	// Then add the strings
	route_string_length += longest_lap_name + longest_lap_kind + longest_transport_name;
	
	// and 8 for the status
	route_string_length += 7;
	
	// Now how much memory do we need for the whole lot?  Let's allocate
	// a buffer.  +1 for the null.
	char* strbuf = malloc((route_string_length * node_count) + 1);
	assert(strbuf != NULL);
	
	// Fill buffer
	char* cursor = strbuf;
	for (curr = &table->list; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
	
		int len = sprintf(cursor, fmt, 
			curr->route.range_start, curr->route.range_end,
			rt_route_lap_name(&curr->route), 
			rt_route_lap_kind(&curr->route),
			rt_route_transport_name(&curr->route),
			curr->route.nexthop.network, curr->route.nexthop.node,
			curr->route.distance,
			rt_route_status_string(curr->status)
		);
		cursor += len;
	}

	xSemaphoreGive(table->mutex);
	return strbuf;
}

size_t rt_count(rt_routing_table_t* table) {
	size_t count = 0;
	struct rt_node_s *curr;
	while (xSemaphoreTake(table->mutex, portMAX_DELAY) != pdTRUE) {}

	for (curr = &table->list; curr != NULL; curr = curr->next) {
		if (curr->dummy) {
			continue;
		}
		count++;
	}

	xSemaphoreGive(table->mutex);
	return count;
}

bool rt_attach_touch_callback(rt_routing_table_t* table, event_callback_t callback) {
	return event_add_callback(&table->touch_event, callback);
}

bool rt_attach_net_range_removed_callback(rt_routing_table_t* table, event_callback_t callback) {
	return event_add_callback(&table->network_range_deleted_event, callback);
}
