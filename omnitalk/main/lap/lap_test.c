#include "lap/lap_test.h"
#include "test.h"

#include <stdbool.h>

#include "mem/buffers.h"
#include "lap/lap.h"

static bool ran_mock;

static bool mockitymockmock(lap_t* lap, buffer_t* buffer) {
	ran_mock = true;
	return true;
}

TEST_FUNCTION(test_lap_lsend_mock) {
	ran_mock = false;
	lap_lsend_mock = &mockitymockmock;
	TEST_ASSERT(lsend(NULL, NULL));
	lap_lsend_mock = NULL;
	TEST_ASSERT(ran_mock);
	TEST_OK();
}
