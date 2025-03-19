#include "web/stats.h"

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <esp_idf_version.h>

#include "web/stats_memory.h"

stats_t stats;
stats_lap_metadata_t stats_lap_metadata[MAX_LAP_COUNT];
stats_omnitalk_metadata_t stats_omnitalk_metadata;

_Atomic(char*) stats_routing_table;
_Atomic(char*) stats_zip_table;

// Have a reserved stats buffer so that we can't run out of memory mid-flow
#define STATSBUFFER_SIZE 256
char statsbuffer[STATSBUFFER_SIZE];

#define COUNTER(R, FIELD, HELP) \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#HELP " #FIELD \
		" %s \n", HELP); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN); \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#TYPE " #FIELD \
		" counter\n" #FIELD " %lu \n", stats.FIELD); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN);

#define COUNTER_LABELS(R, FIELD, SUFFIX, LABELS, HELP) \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#HELP " #FIELD \
		" %s \n", HELP); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN); \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#TYPE " #FIELD \
		" counter\n" #FIELD "{" LABELS "} %lu \n", stats.FIELD##SUFFIX); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN);
	
#define COUNTER_FIELD(R, FIELD, METRIC, LABELS, HELP) \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#HELP " #METRIC \
		" %s \n", HELP); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN); \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#TYPE " #METRIC \
		" counter\n" #METRIC "{" LABELS "} %lu \n", stats.FIELD); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN);

#define GAUGE_FIELD(R, FIELD, METRIC, LABELS, HELP) \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#HELP " #METRIC \
		" %s \n", HELP); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN); \
	snprintf(statsbuffer, STATSBUFFER_SIZE, "#TYPE " #METRIC \
		" gauge\n" #METRIC "{" LABELS "} %lu \n", stats.FIELD); \
	httpd_resp_send_chunk(R, statsbuffer, HTTPD_RESP_USE_STRLEN);


esp_err_t http_metrics_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/plain; version=0.0.4; charset=utf-8");
	
#include "metadata.inc"

	if(stats_routing_table != NULL) {
		httpd_resp_sendstr_chunk(req, stats_routing_table);
	}
	
	if (stats_zip_table != NULL) {
		httpd_resp_sendstr_chunk(req, stats_zip_table);
	}
	
#include "stats.inc"	
	
    httpd_resp_sendstr_chunk(req, NULL);
    
    return ESP_OK;
}

// Housekeeping tasks:

// uptime_task is a task that updates the uptime and a few other stats
// once a second
TaskHandle_t uptime_task;
void uptime_task_runloop(void* dummy) {
	while(1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		stats.uptime_seconds++;
		
		update_memory_stats();		
	}
}

void start_stats(void) {
	// Set our software version in stats
	stats_omnitalk_metadata.git_commit = GIT_VERSION;
	stats_omnitalk_metadata.esp_version = esp_get_idf_version();
	stats_omnitalk_metadata.best_zone = "";
	stats_omnitalk_metadata.ok = true;

	xTaskCreate(uptime_task_runloop, "UPTIME", 768, NULL, tskIDLE_PRIORITY,
		&uptime_task);
}
