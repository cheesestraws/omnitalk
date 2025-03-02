#include "table/aarp/table_test.h"
#include "table/aarp/table.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <lwip/prot/ethernet.h>

size_t count_list_entries_unguarded(struct aarp_node_s *list);

static struct eth_addr test_hwaddr_for_appletalk_address(uint16_t net, uint8_t node) {
	struct eth_addr ea = {
		.addr = {0, 1, 2, (uint8_t)((net >> 8) & 0xff), (uint8_t)(net & 0xff), node }
	};
	
	return ea;
}

TEST_FUNCTION(test_aarp_table) {
	// Creating a table ought to do something
	aarp_table_t* table;
	size_t count;
	bool found;
	struct eth_addr hwaddr, hwaddr2;
	
	table = aarp_new_table();
	TEST_ASSERT(table != NULL);
	TEST_ASSERT(table->mutex != NULL);
	
	count = aarp_table_entry_count(table);
	TEST_ASSERT(count == 0);
	
	// do it again to make sure we release the lock
	count = aarp_table_entry_count(table);
	TEST_ASSERT(count == 0);
	
	// Let's add an entry
	aarp_touch(table, 10, 230, test_hwaddr_for_appletalk_address(10, 230));
	count = aarp_table_entry_count(table);
	TEST_ASSERT(count == 1);
	
	// And another
	aarp_touch(table, 20, 123, test_hwaddr_for_appletalk_address(20, 123));
	count = aarp_table_entry_count(table);
	TEST_ASSERT(count == 2);
	
	// Replace that last one
	aarp_touch(table, 20, 123, test_hwaddr_for_appletalk_address(20, 124));
	count = aarp_table_entry_count(table);
	TEST_ASSERT(count == 2);
	
	// Make sure we're distributing over buckets
	aarp_touch(table, 1, 1, test_hwaddr_for_appletalk_address(1, 1));
	aarp_touch(table, 1, 2, test_hwaddr_for_appletalk_address(1, 2));
	aarp_touch(table, 1, 3, test_hwaddr_for_appletalk_address(1, 3));
	
	int nonzero_buckets = 0;
	for (int i = 0; i < AARP_TABLE_BUCKETS; i++) {
		if (count_list_entries_unguarded(&table->buckets[i]) > 0) {
			nonzero_buckets++;
		}
	}
	TEST_ASSERT(nonzero_buckets > 2);
	
	// Looking up a nonexistent address ought to return false
	found = aarp_lookup(table, 666, 66, &hwaddr);
	TEST_ASSERT(!found);
	
	// Looking up an extant address should retrieve the right MAC address
	found = aarp_lookup(table, 10, 230, &hwaddr);
	TEST_ASSERT(found);
	hwaddr2 = test_hwaddr_for_appletalk_address(10, 230);
	TEST_ASSERT(eth_addr_cmp(&hwaddr, &hwaddr2));
	
	found = aarp_lookup(table, 20, 123, &hwaddr);
	TEST_ASSERT(found);
	hwaddr2 = test_hwaddr_for_appletalk_address(20, 124);
	TEST_ASSERT(eth_addr_cmp(&hwaddr, &hwaddr2));

	TEST_OK();
}
