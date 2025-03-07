#include "table/routing/table_test.h"
#include "table/routing/table.h"

#include "lap/lap_types.h"
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
		
	TEST_OK();
}
