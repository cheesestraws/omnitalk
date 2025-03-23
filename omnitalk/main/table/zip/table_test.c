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
	zt_add_zone_for(table, 5, "Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 1);
	
	// Trying to add the same zone again shouldn't increase counter
	zt_add_zone_for(table, 5, "Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 1);
	
	zt_add_zone_for(table, 5, "Zone2");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 2);

	zt_add_zone_for(table, 5, "Zone3");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, "Zone3");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, "Zone2");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);

	zt_add_zone_for(table, 5, "Zone1");
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	// We've added three zones, so we should have allocated six times
	TEST_ASSERT(stats.mem_all_allocs - old_allocs == 6);
	
	// Adding another network range should be independent
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	old_allocs = stats.mem_all_allocs;
	
	zt_add_zone_for(table, 25, "1enoZ");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 1);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	zt_add_zone_for(table, 25, "2enoZ");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 2);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 3);
	
	zt_add_zone_for(table, 5, "Zone4");
	TEST_ASSERT(zt_count_zones_for(table, 25) == 2);
	TEST_ASSERT(zt_count_zones_for(table, 5) == 4);
	
	// We've added three more zones, so we should have allocated six times
	TEST_ASSERT(stats.mem_all_allocs - old_allocs == 6);
	
	// Now remove a network, and check that it's freed the zones
	old_frees = stats.mem_all_frees;
	TEST_ASSERT(zt_delete_network(table, 5));
	
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
	zt_add_zone_for(table, 20, "Zone1");
	zt_add_zone_for(table, 20, "Zone2");
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(!zt_network_is_complete(table, 20));

	// "Adding" the same two again shouldn't affect the unique zone count and thus
	// shouldn't make completeness happen
	zt_add_zone_for(table, 20, "Zone1");
	zt_add_zone_for(table, 20, "Zone2");
	zt_check_zone_count_for_completeness(table, 20);
	TEST_ASSERT(!zt_network_is_complete(table, 20));
	
	// Adding two new ones, though, should.
	zt_add_zone_for(table, 20, "Zone3");
	zt_add_zone_for(table, 20, "Zone4");
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
	
	zt_add_zone_for(table, 20, "Zone1");
	zt_add_zone_for(table, 20, "Zone2");
	zt_add_zone_for(table, 20, "Zone3");
	zt_add_zone_for(table, 20, "Zone4");

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

static bool test_iter_loop(void* pvt, int idx, uint16_t network, char* zone) {
	test_zip_iteration_state_t *state = (test_zip_iteration_state_t*)pvt;
	SET_TEST_NAME(state->test_name);
	
	TEST_ASSERT(idx == state->next_expected_index);
	state->next_expected_index++;
	
	state->iterations++;
	state->cursor = stpcpy(state->cursor, zone);
		
	// simulate an error
	if (strcmp(zone, "3BREAK") == 0) {
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

TEST_FUNCTION(test_zip_table_iteration) {
	// create ourselves a table
	zt_zip_table_t* table = zt_new();
	TEST_ASSERT(zt_add_net_range(table, 20, 30));
	
	zt_add_zone_for(table, 20, "Zone1");
	zt_add_zone_for(table, 20, "Zone2");
	zt_add_zone_for(table, 20, "Zone3");
	zt_add_zone_for(table, 20, "Zone4");
	zt_mark_network_complete(table, 20);
	
	TEST_ASSERT(zt_add_net_range(table, 1, 10));
	zt_add_zone_for(table, 1, "1enoZ");
	zt_add_zone_for(table, 1, "2enoZ");
	zt_add_zone_for(table, 1, "3BREAK");
	zt_add_zone_for(table, 1, "4enoZ");
	zt_add_zone_for(table, 1, "5enoZ");

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

	TEST_OK();
}
