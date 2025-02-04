#include "web/stats.h"

#include <esp_err.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


stats_t stats;

// Have a reserved stats buffer so that we can't run out of memory mid-flow
#define STATSBUFFER_SIZE 256
char statsbuffer[STATSBUFFER_SIZE];

#define COUNTER(R, FIELD, HELP) \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#HELP " #FIELD \
		" " #FIELD " %s \n", HELP); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN); \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#TYPE " #FIELD \
		" counter\n" #FIELD " %lu \n", stats.FIELD); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN);


esp_err_t http_metrics_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/plain; version=0.0.4; charset=utf-8");

	COUNTER(req, uptime_seconds, "system uptime in seconds")
	
    httpd_resp_sendstr_chunk(req, NULL);
    
    return ESP_OK;
}

// Housekeeping tasks:

// uptime_task is a task that just updates the uptime
// once a second
TaskHandle_t uptime_task;
void uptime_task_runloop(void* dummy) {
	while(1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		stats.uptime_seconds++;
	}
}

void start_stats_workers(void) {
	xTaskCreate(uptime_task_runloop, "UPTIME", 256, NULL, tskIDLE_PRIORITY,
		&uptime_task);
}
