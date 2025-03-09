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
	bool result;
	rt_route_t out = { 0 };
	rt_route_t in = { 0 };
	rt_routing_table_t* table = rt_new();
	
	// Add an RTMP-learned route.
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_1,
		.distance = 10,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_1);
	TEST_ASSERT(out.distance == 10);
	TEST_ASSERT(rt_count(table) == 1);
	
	// If we change the distance, but leave the LAP and nexthop the same,
	// then the route should be replaced and the count should not increase.
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_1,
		.distance = 20,
	};
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_1);
	TEST_ASSERT(out.distance == 20);
	TEST_ASSERT(rt_count(table) == 1);
	
	// But, if we add one with another LAP, the count should tick up
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_2,
		.distance = 20,
	};
	rt_touch(table, in);
	TEST_ASSERT(rt_count(table) == 2);
	
	// Now, if we prune the table a couple of times to give the old routes time
	// to become bad, and re-touch the new one, we should get the new one returned
	// and the old ones will fall out
	rt_prune(table); 
	rt_prune(table);
	rt_touch(table, in);
	result = rt_lookup(table, 15, &out);
	TEST_ASSERT(result);
	TEST_ASSERT(out.outbound_lap == &canary_lap_2);
	TEST_ASSERT(out.distance == 20);
	TEST_ASSERT(rt_count(table) == 2);
	
	rt_prune(table);
	TEST_ASSERT(rt_count(table) == 1);

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

static int touch_event_called = 0;
static int delete_event_called = 0;
static rt_route_t munged_route;

static void test_touch_event_callback(void* route_ptr) {
	touch_event_called++;
	munged_route = *(rt_route_t*)route_ptr;
}

static void test_delete_event_callback(void* route_ptr) {
	delete_event_called++;
	munged_route = *(rt_route_t*)route_ptr;
}


TEST_FUNCTION(test_routing_table_events) {
	rt_route_t in;
	rt_routing_table_t* table = rt_new();
	
	// attach some quick callbacks
	rt_attach_touch_callback(table, &test_touch_event_callback);
	rt_attach_net_range_removed_callback(table, &test_delete_event_callback);


	// direct routes should call callback	
	rt_touch_direct(table, 1, 4, &canary_lap_1);
	TEST_ASSERT(touch_event_called == 1);
	TEST_ASSERT(delete_event_called == 0);

	
	// Add an RTMP-learned route.  Check callback called
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_2,
		.distance = 10,
	};
	rt_touch(table, in);
	TEST_ASSERT(touch_event_called == 2);
	TEST_ASSERT(delete_event_called == 0);
	TEST_ASSERT(rt_routes_equal(&in, &munged_route));
	munged_route = (rt_route_t){ 0 };

	// Wait for it to fall out of the table
	rt_prune(table);
	rt_prune(table);
	rt_prune(table);
	
	// and verify that it has: 1 is our direct route from earlier
	TEST_ASSERT(rt_count(table) == 1);
	
	// There will be no other route for this net range, so we should have fired the
	// delete event.
	TEST_ASSERT(touch_event_called == 2);
	TEST_ASSERT(delete_event_called == 1);
	// the route will have been demoted
	in.distance = 31;
	TEST_ASSERT(rt_routes_equal(&in, &munged_route));
	
	
	// But if there's another route, we shouldn't fire the deleted net-range
	// callback.  Let's add another RTMP route
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_2,
		.distance = 10,
	};
	rt_touch(table, in);
	
	// Let it age to being suspect, then introduce a second route with a different LAP
	// and distance
	rt_prune(table);
	in = (rt_route_t){
		.range_start = 10,
		.range_end = 20,
		.outbound_lap = &canary_lap_3,
		.distance = 20,
	};
	rt_touch(table, in);
	
	TEST_ASSERT(rt_count(table) == 3);
	
	// Now age out our first route.  No delete should fire.
	rt_prune(table);
	rt_prune(table);
	TEST_ASSERT(rt_count(table) == 2);
	TEST_ASSERT(delete_event_called == 1);
	
	// Then age out the second; only now should our delete fire
	rt_prune(table);
	TEST_ASSERT(rt_count(table) == 1);
	TEST_ASSERT(delete_event_called == 2);
	in.distance = 31;
	TEST_ASSERT(rt_routes_equal(&in, &munged_route));

	TEST_OK();
}
