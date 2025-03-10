#include "table/zip/table_test.h"
#include "table/zip/table.h"

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
