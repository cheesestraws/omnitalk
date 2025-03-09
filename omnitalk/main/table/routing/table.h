#pragma once

#include <stdbool.h>

#include "lap/lap_types.h"
#include "table/routing/table_impl.h"
#include "util/event/event.h"

// This file contains the public interface of the RIB.
// You can assume the existence of routing_table_t but you should assume nothing
// else about it.

rt_routing_table_t* rt_new();
void rt_touch(rt_routing_table_t* table, rt_route_t r);
void rt_touch_direct(rt_routing_table_t* table, uint16_t start, uint16_t end, lap_t *lap);
bool rt_lookup(rt_routing_table_t* table, uint16_t network_number, rt_route_t *out);
void rt_prune(rt_routing_table_t* table);
char* rt_stats(rt_routing_table_t* table);
void rt_print(rt_routing_table_t* table);
size_t rt_count(rt_routing_table_t* table);

// Attach an event handler for when a route is touched.  You MUST NOT attempt to alter
// the routing table in the event handler for this callback.  Your event handler will be
// passed the route that was touched.  You MUST NOT modify this route.
bool rt_attach_touch_callback(rt_routing_table_t* table, event_callback_t callback);

// Attach an event handler for when a network range is completely removed from the table.
// You MUST NOT attempt to alter the routing table in the event handler for this callback.
// Your event handler will be passed the route that was removed to finally remove the
// net range from the table.  You MUST NOT modify this route.
bool rt_attach_net_range_removed_callback(rt_routing_table_t* table, event_callback_t callback);
