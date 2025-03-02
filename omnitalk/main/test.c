#include "test.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "test_main";

TEST_FUNCTION(test_tests) {
	TEST_OK();
}

void real_test_ok(char* test_name) {
	ESP_LOGI(test_name, "OK");
}

void real_test_fail(char* test_name, char* msg) {
	ESP_LOGE(test_name, "FAIL: %s", msg);
	vTaskDelay(portMAX_DELAY);
}

void test_main(void) {
	printf("\n\n\n");
	ESP_LOGI(TAG, "Running unit tests");
	ESP_LOGI(TAG, "==================");
	printf("\n");
	
#include "test.inc"
	
	printf("\n\n\n");
}
