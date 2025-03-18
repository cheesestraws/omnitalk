#include "registry_test.h"

#include "lap/registry.h"
#include "test.h"

TEST_FUNCTION(test_lap_registry_ordering) {
	lap_registry_t *reg = lap_registry_new();
	TEST_ASSERT(reg != NULL);
	
	lap_t good_lap = { .id = 3, .quality = 1 };
	lap_t better_lap = { .id = 2, .quality = 2 };
	lap_t best_lap = { .id = 1, .quality = 3 };

	// Register the middle LAP first
	lap_registry_register(reg, &better_lap);
	TEST_ASSERT(lap_registry_highest_quality_lap(reg) == &better_lap);
	TEST_ASSERT(lap_registry_lap_count(reg) == 1);
	
	// Register a worse LAP; the highest Q lap should still be the better one
	lap_registry_register(reg, &good_lap);
	TEST_ASSERT(lap_registry_highest_quality_lap(reg) == &better_lap);
	TEST_ASSERT(lap_registry_lap_count(reg) == 2);

	// Register a better; it should take priority.
	lap_registry_register(reg, &best_lap);
	TEST_ASSERT(lap_registry_highest_quality_lap(reg) == &best_lap);
	TEST_ASSERT(lap_registry_lap_count(reg) == 3);
	
	TEST_OK();
}
