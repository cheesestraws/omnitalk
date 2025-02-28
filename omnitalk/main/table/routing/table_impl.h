#pragma once

#include <stdbool.h>

#include "table/routing/route.h"

// This file contains a really stupid routing table.  We can replace it with something
// faster later if we need to.
//
// This table is just a linked list, ordered approximately by distance.

struct rt_node_s {
	// Inserting a dummy node at the start of the list because it makes deleting easier.
	bool dummy;
	
	rt_route_t route;
	int64_t last_touched_timestamp;
	
	struct rt_node_s *next;
};

typedef struct {
	SemaphoreHandle_t mutex;
	
	struct rt_node_s list;
} rt_routing_table_t;
