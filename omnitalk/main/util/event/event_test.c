#include "util/event/event_test.h"
#include "util/event/event.h"

static void callback_1(void* param) {
	*((int*)param) |= 1;
}

static void callback_2(void* param) {
	*((int*)param) |= 2;
}

static void callback_3(void* param) {
	*((int*)param) |= 4;
}

static void callback_4(void* param) {
	*((int*)param) |= 8;
}

TEST_FUNCTION(test_event_add_callback) {
	event_t evt = { 0 };
	// We should be able to add up to EVENT_MAX_CALLBACKS callbacks
	
	for (int i = 0; i < EVENT_MAX_CALLBACKS; i++) {
		TEST_ASSERT(event_add_callback(&evt, &callback_1));
	}
	
	// Then we shouldn't be able to add any more
	TEST_ASSERT(!event_add_callback(&evt, &callback_1));

	TEST_OK();
}

TEST_FUNCTION(test_event_fire) {
	event_t evt = { 0 };
	int result = 0;
	
	TEST_ASSERT(event_add_callback(&evt, &callback_1));
	TEST_ASSERT(event_add_callback(&evt, &callback_2));
	TEST_ASSERT(event_add_callback(&evt, &callback_3));
	TEST_ASSERT(event_add_callback(&evt, &callback_4));
	event_fire(&evt, &result);
	
	TEST_ASSERT(result == 15);

	TEST_OK();
}
