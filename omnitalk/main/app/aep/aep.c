#include "app/aep/aep.h"

#include <esp_log.h>

#include "mem/buffers.h"

static const char* TAG = "AEP";

void app_aep_handler(buffer_t *packet) {
	ESP_LOGI(TAG, "got AEP packet");
	freebuf(packet);
}
