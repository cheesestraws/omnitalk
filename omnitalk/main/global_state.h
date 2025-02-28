#pragma once

#include "table/routing/table.h"

// Yeah, yeah, global state is icky, but it's easier to have a single routing table
// here.
extern rt_routing_table_t *global_routing_table;
