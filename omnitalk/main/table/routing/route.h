#pragma once

#include <stdbool.h>

#include "lap/lap.h"

typedef struct {
	uint16_t network;
	uint8_t node;
} rt_nexthop_t;

typedef struct {
	uint16_t range_start;
	uint16_t range_end;
	
	lap_t *outbound_lap;
	rt_nexthop_t nexthop;
	
	uint8_t distance;
} rt_route_t;

static inline bool rt_routes_equal(rt_route_t *a, rt_route_t *b) {
	return a->range_start == b->range_start &&
		a->range_end == b->range_end &&
		a->outbound_lap == b->outbound_lap &&
		a->nexthop.network == b->nexthop.network &&
		a->nexthop.node == b->nexthop.node &&
		a->distance == b->distance;
}

// rt_routes_match check that everything in the route is the same
// except for the distance: this allows other routers to change the
// distance to a network
static inline bool rt_routes_match(rt_route_t *a, rt_route_t *b) {
	return a->range_start == b->range_start &&
		a->range_end == b->range_end &&
		a->outbound_lap == b->outbound_lap &&
		a->nexthop.network == b->nexthop.network &&
		a->nexthop.node == b->nexthop.node;
}
