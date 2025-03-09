#include "test.h"

#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "test.inc.h"
#include "tunables.h"

static const char* TAG = "test_main";

TEST_FUNCTION(test_tests) {
	TEST_OK();
}

void real_test_ok(char* test_name) {
	ESP_LOGI(TAG, "[TEST %s] OK", test_name);
}

void real_test_fail(char* test_name, char* msg) {
	ESP_LOGE(TAG, "[TEST %s] FAIL: %s", test_name, msg);
#ifndef TESTS_NO_HANG_ON_FAIL
	vTaskDelay(portMAX_DELAY);
#endif
}

void test_main(void) {
	printf("\n\n\n");
	ESP_LOGI(TAG, "Running unit tests");
	ESP_LOGI(TAG, "==================");
	printf("\n");
	
#include "test.inc"
	
	printf("\n\n\n");
	
#ifdef TESTS_REBOOT_AFTERWARDS
	esp_restart();
#endif
}
