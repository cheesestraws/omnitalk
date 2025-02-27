#pragma once

#include <stdbool.h>

#include "table/routing/table_impl.h"

// This file contains the public interface of the RIB.
// You can assume the existence of routing_table_t but you should assume nothing
// else about it.

rt_routing_table_t* rt_new();
void rt_touch(rt_routing_table_t* table, rt_route_t r);
bool rt_lookup(rt_routing_table_t* table, uint16_t network_number, rt_route_t *out);
void rt_print(rt_routing_table_t* table);
