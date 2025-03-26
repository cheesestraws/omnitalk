#include "table/zip/table_test.h"
#include "table/zip/table.h"

#include <stdbool.h>

#include "web/stats.h"
#include "test.h"

TEST_FUNCTION(test_zip_table_networks) {
	long old_frees;
	
	zt_zip_table_t* table = zt_new();
	
	// A newly created table should have no networks in it.
	TEST_ASSERT(zt_count_net_ranges(table) == 0);
	
	// We can add a network range; it will return true to say we've added it.
	TEST_ASSERT(zt_add_net_range(table, 5, 10));
	TEST_ASSERT(zt_count_net_ranges(table) == 1);
	TEST_ASSERT(zt_contains_net(table, 7));
	
	// Adding it again should return false and not do anything at all
	TEST_ASSERT(!zt_add_net_range(table, 5, 10));
	TEST_ASSERT(zt_count_net_ranges(table) == 1);
	TEST_ASSERT(zt_contains_net(table, 7));
	
	// Adding a different one should succeed though
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	TEST_ASSERT(zt_count_net_ranges(table) == 2);
	TEST_ASSERT(zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	
	// And re-adding the first one should still be a noop
	TEST_ASSERT(!zt_add_net_range(table, 5, 10));
	TEST_ASSERT(zt_count_net_ranges(table) == 2);
	TEST_ASSERT(zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	
	// This should also be the case if our net_start is lower:
	TEST_ASSERT(zt_add_net_range(table, 1, 3));
	TEST_ASSERT(zt_count_net_ranges(table) == 3);
	TEST_ASSERT(zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	TEST_ASSERT(zt_contains_net(table, 2));
	
	TEST_ASSERT(!zt_add_net_range(table, 5, 10));
	TEST_ASSERT(zt_count_net_ranges(table) == 3);
	TEST_ASSERT(zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	TEST_ASSERT(zt_contains_net(table, 2));
	
	// Now we should be able to delete the first one, in the middle.
	// Its network node should be freed
	old_frees = stats.mem_all_frees;
	TEST_ASSERT(zt_delete_network(table, 5));
	TEST_ASSERT(zt_count_net_ranges(table) == 2);
	TEST_ASSERT(!zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	TEST_ASSERT(zt_contains_net(table, 2));
	TEST_ASSERT(stats.mem_all_frees == old_frees + 1);

	// Deleting again should return false
	TEST_ASSERT(!zt_delete_network(table, 5));
	TEST_ASSERT(zt_count_net_ranges(table) == 2);
	TEST_ASSERT(!zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	TEST_ASSERT(zt_contains_net(table, 2));
	
	// Likewise we should be able to delete the first.
	old_frees = stats.mem_all_frees;
	TEST_ASSERT(zt_delete_network(table, 1));
	TEST_ASSERT(zt_count_net_ranges(table) == 1);
	TEST_ASSERT(!zt_contains_net(table, 7));
	TEST_ASSERT(zt_contains_net(table, 21));
	TEST_ASSERT(!zt_contains_net(table, 2));
	TEST_ASSERT(stats.mem_all_frees == old_frees + 1);

	TEST_OK();
}

TEST_FUNCTION(test_zip_table_zones) {
	long old_allocs;
	long old_frees;
	zt_zip_table_t* table = zt_new();
	
	TEST_ASSERT(zt_add_net_range(table, 5, 10));
	
	old_allocs = stats.mem_all_allocs;
	
	// Add a zone and check it got added
	zt_add_zone_for(table, 5, (pstring*)"\x05Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 1);
	
	// Trying to add the same zone again shouldn't increase counter
	zt_add_zone_for(table, 5, (pstring*)"\x05Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 1);
	
	zt_add_zone_for(table, 5, (pstring*)"\x05Zone2");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 2);

	zt_add_zone_for(table, 5, (pstring*)"\x05Zone3");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, (pstring*)"\x05Zone3");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, (pstring*)"\x05Zone2");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, (pstring*)"\x05Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	// We've added three zones, so we should have allocated six times
	TEST_ASSERT(stats.mem_all_allocs - old_allocs == 6);
	
	// Adding another network range should be independent
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	old_allocs = stats.mem_all_allocs;
	
	zt_add_zone_for(table, 25, (pstring*)"\x05""1enoZ");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 1);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	zt_add_zone_for(table, 25, (pstring*)"\x05""2enoZ");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 2);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	zt_add_zone_for(table, 5, (pstring*)"\x05Zone4");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 2);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 4);
	
	// We've added three more zones, so we should have allocated six times
	TEST_ASSERT(stats.mem_all_allocs - old_allocs == 6);
	
	// can we count all nodes?
	TEST_ASSERT(zt_count_all_zones(table) == 0);
	zt_mark_network_complete(table, 5);
	TEST_ASSERT(zt_count_all_zones(table) == 4);
	zt_mark_network_complete(table, 25);
	TEST_ASSERT(zt_count_all_zones(table) == 6);
	
	// Now remove a network, and check that it's freed the zones
	old_frees = stats.mem_all_frees;
	TEST_ASSERT(zt_delete_network(table, 5));
	TEST_ASSERT(zt_count_all_zones(table) == 2);
	
	// There are four zones associated with network 5, and one alloc
	// for the network node itself (2 * 4 + 1), so:
	TEST_ASSERT(stats.mem_all_frees - old_frees == 9);
	
	
	TEST_OK();
}

TEST_FUNCTION(test_zip_table_completion) {
	zt_zip_table_t* table = zt_new();
	
	TEST_ASSERT(zt_add_net_range(table, 5, 10));
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	// marking a network as complete should only mark that network as complete
	zt_mark_network_complete(table, 5);
	TEST_ASSERT(zt_network_is_complete(table, 5));
	
	// and not the other
	TEST_ASSERT(!zt_network_is_complete(table, 20));
	
	// Now let's mark 20 as expecting 4 zones; it will get this information from the
	// "network count" field of an extended ZIP reply (which isn't actually a network
	// count, it's a zone count).
	zt_set_expected_zone_count(table, 20, 4);
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(!zt_network_is_complete(table, 20));
	
	// Add a couple of zones and we still shouldn't be complete
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone1");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone2");
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(!zt_network_is_complete(table, 20));

	// "Adding" the same two again shouldn't affect the unique zone count and thus
	// shouldn't make completeness happen
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone1");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone2");
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(!zt_network_is_complete(table, 20));
	
	// Adding two new ones, though, should.
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone3");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone4");
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(zt_network_is_complete(table, 20));

	TEST_OK();
}

TEST_FUNCTION(test_zip_table_stats) {
	long old_allocations;
	long allocations;
	// Make sure our stats don't leak memory
	zt_zip_table_t* table = zt_new();
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone1");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone2");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone3");
	zt_add_zone_for(table, 20, (pstring*)"\x05Zone4");

	// We should only have one "loose" allocation, which is the char* we return
	old_allocations = stats.mem_all_allocs - stats.mem_all_frees;
	char* ststr = zt_stats(table);
	allocations = stats.mem_all_allocs - stats.mem_all_frees;
	TEST_ASSERT(allocations - old_allocations == 1);
	
	free(ststr);
	TEST_OK();
}

typedef struct {
	char* test_name;

	bool expected_exists;
	int expected_zone_count;
	bool expected_complete;
	
	bool inited;
	
	int next_expected_index;
	int iterations;
	char buffer[256];
	char* cursor;
	
	bool expected_aborted;
	bool finalised;
} test_zip_iteration_state_t;

static bool test_iter_init(void* pvt, uint16_t network, bool exists, size_t zone_count, bool complete) {
	test_zip_iteration_state_t *state = (test_zip_iteration_state_t*)pvt;
	SET_TEST_NAME(state->test_name);
	
	TEST_ASSERT(exists == state->expected_exists);
	TEST_ASSERT(zone_count == state->expected_zone_count);
	TEST_ASSERT(complete == state->expected_complete);
	
	state->inited = true;
	
	return true;
}

static bool test_iter_loop(void* pvt, int idx, uint16_t network, pstring* zone) {
	test_zip_iteration_state_t *state = (test_zip_iteration_state_t*)pvt;
	SET_TEST_NAME(state->test_name);
	
	TEST_ASSERT(idx == state->next_expected_index);
	state->next_expected_index++;
	
	state->iterations++;
	// We've cheated and made all our zone strings both C and pascal strings, so we can
	// just point to the first char of zone to get the c string (with a null on the end)
	char* cstr = &zone->str[0];
	state->cursor = stpcpy(state->cursor, cstr);
		
	// simulate an error
	if (strcmp(cstr, "3BREAK") == 0) {
		return false;
	} else {
		return true;
	}
}

static bool test_iter_end(void* pvt, bool aborted) {
	test_zip_iteration_state_t *state = (test_zip_iteration_state_t*)pvt;
	SET_TEST_NAME(state->test_name);
		
	TEST_ASSERT(aborted == state->expected_aborted);
	state->finalised = true;
	
	return !aborted;
}

struct zone_name_iter_state {
	int break_out_after;
	int iterations;
	bool seen_null;
	char buffer[256];
	char* cursor;
};

static bool simple_zone_name_iterator(void* pvt, pstring* zone) {
	struct zone_name_iter_state *state = (struct zone_name_iter_state*)pvt;
	
	if (state->cursor == NULL) {
		state->cursor = state->buffer;
	}
	
	if (zone == NULL) {
		state->seen_null = true;
		return true;
	}
	
	state->iterations++;
	// We've cheated and made all our zone strings both C and pascal strings, so we can
	// just point to the first char of zone to get the c string (with a null on the end)
	char* cstr = &zone->str[0];
	state->cursor = stpcpy(state->cursor, cstr);
	
	if (state->iterations == state->break_out_after) {
		return false;
	}
	
	return true;
}

TEST_FUNCTION(test_zip_table_iteration) {
	// create ourselves a table
	zt_zip_table_t* table = zt_new();
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	zt_add_zone_for(table, 20, (pstring*)"\x06Zone1");
	zt_add_zone_for(table, 20, (pstring*)"\x06Zone2");
	zt_add_zone_for(table, 20, (pstring*)"\x06Zone3");
	zt_add_zone_for(table, 20, (pstring*)"\x06Zone4");
	zt_mark_network_complete(table, 20);
	
	TEST_ASSERT(zt_add_net_range(table, 1, 10));
	zt_add_zone_for(table, 1, (pstring*)"\x06""1enoZ");
	zt_add_zone_for(table, 1, (pstring*)"\x06""2enoZ");
	zt_add_zone_for(table, 1, (pstring*)"\x07""3BREAK");
	zt_add_zone_for(table, 1, (pstring*)"\x06""4enoZ");
	zt_add_zone_for(table, 1, (pstring*)"\x06""5enoZ");

	test_zip_iteration_state_t state;
	
	// the state here looks scary but really we're just keeping track of everything
	state = (test_zip_iteration_state_t){
		.test_name = TEST_NAME,
		.expected_zone_count = 4,
		.expected_complete = true,
		.expected_exists = true,
		.expected_aborted = false,
	};
	state.cursor = &state.buffer[0];
	
	// Iterate over that table
	TEST_ASSERT(zt_iterate_net(table, &state, 20, test_iter_init, test_iter_loop, test_iter_end));
	
	// Did we do the right stuff?
	TEST_ASSERT(state.inited);
	TEST_ASSERT(state.iterations == 4);
	TEST_ASSERT(state.finalised);
	TEST_ASSERT(strcmp(state.buffer, "Zone1Zone2Zone3Zone4") == 0);

	// Now let's run another test over one that should error
	state = (test_zip_iteration_state_t){
		.test_name = TEST_NAME,
		.expected_zone_count = 5,
		.expected_complete = false,
		.expected_exists = true,
		.expected_aborted = true,
	};
	state.cursor = &state.buffer[0];
	// iterator should return false
	TEST_ASSERT(!zt_iterate_net(table, &state, 1, test_iter_init, test_iter_loop, test_iter_end));
	
	// Did we do the right stuff?
	TEST_ASSERT(state.inited);
	TEST_ASSERT(state.iterations == 3);
	TEST_ASSERT(state.finalised);
	TEST_ASSERT(strcmp(state.buffer, "1enoZ2enoZ3BREAK") == 0);
	
	// Now let's try out the zone name iterator.
	// Add another complete netrange at the end
	TEST_ASSERT(zt_add_net_range(table, 40, 50));
	zt_add_zone_for(table, 40, (pstring*)"\x06ZoneA");
	zt_add_zone_for(table, 40, (pstring*)"\x06ZoneB");
	zt_add_zone_for(table, 40, (pstring*)"\x06ZoneC");
	zt_add_zone_for(table, 40, (pstring*)"\x06ZoneD");
	zt_mark_network_complete(table, 40);
	
	struct zone_name_iter_state state2 = { 0 };
	zt_iterate_zone_names(table, &state2, 0, simple_zone_name_iterator);
	// Make sure we skip the incomplete network range
	TEST_ASSERT(strcmp(state2.buffer, "Zone1Zone2Zone3Zone4ZoneAZoneBZoneCZoneD") == 0);
	TEST_ASSERT(state2.seen_null);
		
	bzero(&state2, sizeof(state2));
	zt_iterate_zone_names(table, &state2, 2, simple_zone_name_iterator);
	TEST_ASSERT(strcmp(state2.buffer, "Zone3Zone4ZoneAZoneBZoneCZoneD") == 0);
	TEST_ASSERT(state2.seen_null);
	
	bzero(&state2, sizeof(state2));
	zt_iterate_zone_names(table, &state2, 5, simple_zone_name_iterator);
	TEST_ASSERT(strcmp(state2.buffer, "ZoneBZoneCZoneD") == 0);
	TEST_ASSERT(state2.seen_null);

	// Does bailing out early work?
	bzero(&state2, sizeof(state2));
	state2.break_out_after = 4;
	zt_iterate_zone_names(table, &state2, 2, simple_zone_name_iterator);
	TEST_ASSERT(strcmp(state2.buffer, "Zone3Zone4ZoneAZoneB") == 0);
	TEST_ASSERT(!state2.seen_null);

	// What about iterating through individual networks?
	bzero(&state2, sizeof(state2));
	TEST_ASSERT(zt_iterate_zone_names_for_net(table, &state2, 20, 0, simple_zone_name_iterator));
	TEST_ASSERT(strcmp(state2.buffer, "Zone1Zone2Zone3Zone4") == 0);
	TEST_ASSERT(state2.seen_null);
	
	// Starting at 2 in net 20 shouldn't overrun into the next net
	bzero(&state2, sizeof(state2));
	TEST_ASSERT(zt_iterate_zone_names_for_net(table, &state2, 20, 2, simple_zone_name_iterator));
	TEST_ASSERT(strcmp(state2.buffer, "Zone3Zone4") == 0);
	TEST_ASSERT(state2.seen_null);
	
	// Starting at 2 in net 20 shouldn't overrun into the next net
	bzero(&state2, sizeof(state2));
	state2.break_out_after = 1;
	TEST_ASSERT(zt_iterate_zone_names_for_net(table, &state2, 20, 2, simple_zone_name_iterator));
	TEST_ASSERT(strcmp(state2.buffer, "Zone3") == 0);
	TEST_ASSERT(!state2.seen_null);
		
	// net 1 isn't ready
	bzero(&state2, sizeof(state2));
	TEST_ASSERT(!zt_iterate_zone_names_for_net(table, &state2, 1, 0, simple_zone_name_iterator));
	TEST_ASSERT(strcmp(state2.buffer, "") == 0);
	TEST_ASSERT(!state2.seen_null);
	
	// net 100 doesn't exist
	bzero(&state2, sizeof(state2));
	TEST_ASSERT(!zt_iterate_zone_names_for_net(table, &state2, 1, 0, simple_zone_name_iterator));
	TEST_ASSERT(strcmp(state2.buffer, "") == 0);
	TEST_ASSERT(!state2.seen_null);

	TEST_OK();
}

static bool test_null_iter_loop(void* pvt, int idx, uint16_t network, pstring* zone) {
	SET_TEST_NAME((char*)pvt);
	
	TEST_ASSERT(pstring_eq_pstring(zone, (pstring*)"\x0dhorrible\0name"));
	
	return true;
}

TEST_FUNCTION(test_zip_table_zone_names_with_null_in) {
	zt_zip_table_t* table = zt_new();
	TEST_ASSERT(zt_add_net_range(table, 20, 30));

	pstring* zone_name = (pstring*)"\x0dhorrible\0name";
	zt_add_zone_for(table, 20, zone_name);
	
	TEST_ASSERT(zt_iterate_net(table, TEST_NAME, 20, NULL, test_null_iter_loop, NULL));

	TEST_OK();
}
