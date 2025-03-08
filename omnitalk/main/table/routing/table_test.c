#include "table/routing/table_test.h"
#include "table/routing/table.h"

#include "lap/lap_types.h"
#include "web/stats.h"
#include "test.h"

// A couple of empty LAPs we can use as a marker value
static lap_t canary_lap_1;
static lap_t canary_lap_2;
static lap_t canary_lap_3;
static lap_t canary_lap_4;

TEST_FUNCTION(test_routing_table_basics) {
	bool result;
	rt_route_t out = { 0 };
	rt_route_t in = { 0 };
	rt_routing_table_t* table = rt_new();
	
	// Looking up a nonexistent route in an empty table should not return anything
	result = rt_lookup(table, 666, &out);
	TEST_ASSERT(!result);
	
	// Adding a direct route and querying for that route should return it
	rt_touch_direct(table, 1, 4, &canary_lap_1);
	result = rt_lookup(table, 3, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(rt_count(table) == 1);
	
	// Direct routes have distance = 0
	TEST_ASSERT(out.distance == 0);
	TEST_ASSERT(out.range_start == 1);
	TEST_ASSERT(out.range_end == 4);
	TEST_ASSERT(out.outbound_lap == &canary_lap_1);
	
	// Looking up a nonexistent route should *still* not return anything
	result = rt_lookup(table, 666, &out);
	TEST_ASSERT(!result);

	// We can add an RTMP-learned route shadowing this route
	in = (rt_route_t){
		.range_start = 1,
		.range_end = 4,
		.outbound_lap = &canary_lap_2,
		.distance = 2,
	};
	rt_touch(table, in);
	TEST_ASSERT(rt_count(table) == 2);
	
	// But looking up the route for this should return the direct route still
	result = rt_lookup(table, 3, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_1);

	// If we just add an RTMP-learned route, we should be able to find it:
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_3,
		.distance = 10,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_3);
	TEST_ASSERT(rt_count(table) == 3);

	// But if we add a closer RTMP-learned route, that should take precedence
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_4,
		.distance = 5,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_4);
	TEST_ASSERT(rt_count(table) == 4);
		
	TEST_OK();
}

TEST_FUNCTION(test_routing_table_distance_and_replacement) {
	TEST_OK();
}

TEST_FUNCTION(test_routing_table_aging) {
	bool result;
	rt_route_t out = { 0 };
	rt_route_t in = { 0 };
	rt_routing_table_t* table = rt_new();

	// Direct routes shouldn't age out, even after three prunings.
	out.distance = 10; // so we can check it's set to 0 later
	rt_touch_direct(table, 1, 4, &canary_lap_1);
	rt_prune(table); rt_prune(table); rt_prune(table);
	result = rt_lookup(table, 3, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.distance == 0);
	
	// Add an RTMP-learned route
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_2,
		.distance = 10,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 10);
	
	// After a single prune, we should still have the route
	rt_prune(table);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 10);

	// After a second prune with no touch, we should have a bad route
	// with distance=31 (per rtmp specs)
	rt_prune(table);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 31);
	
	// And after a third prune, the route should be gone and we should have freed
	// its memory.
	long old_frees = stats.mem_all_frees;
	rt_prune(table);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(!result);
	TEST_ASSERT(stats.mem_all_frees > old_frees);
	
	// We're going to put it back, but this time, we're going to touch it again after
	// the first prune, and this should then require three *further* prunes to kill it.
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_2,
		.distance = 10,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 10);

	// Prune and re-touch
	rt_prune(table);
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 10);

	// Now if we prune two more times, for a total of three since the route was
	// inserted, we should get a bad route, rather than a gone route.
	rt_prune(table);
	rt_prune(table);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 31);
		
	TEST_OK();
}
