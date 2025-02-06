#include "web/stats.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>

#include "web/stats_memory.h"


stats_t stats;

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


esp_err_t http_metrics_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/plain; version=0.0.4; charset=utf-8");

	COUNTER(req, uptime_seconds, "system uptime in seconds");
	COUNTER(req, mem_all_allocs, "all heap allocations");
	COUNTER(req, mem_all_frees, "all heap allocations");
	
	COUNTER_LABELS(req, mem_total_free_bytes, __type_heap,
		"type=\"heap\"", "total free bytes");
	COUNTER_LABELS(req, mem_minimum_free_bytes, __type_heap,
		"type=\"heap\"", "total free bytes");
	COUNTER_LABELS(req, mem_largest_free_block, __type_heap,
		"type=\"heap\"", "total free bytes");
	COUNTER_LABELS(req, mem_total_free_bytes, __type_dma,
		"type=\"dma\"", "total free bytes");
	COUNTER_LABELS(req, mem_minimum_free_bytes, __type_dma,
		"type=\"dma\"", "total free bytes");
	COUNTER_LABELS(req, mem_largest_free_block, __type_dma,
		"type=\"dma\"", "total free bytes");

	COUNTER(req, eth_recv_elap_frames, "ethernet: received ELAP frames (raw count)");
	COUNTER(req, eth_recv_aarp_frames, "ethernet: received AARP frames (raw count)");
	
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

void start_stats_workers(void) {
	xTaskCreate(uptime_task_runloop, "UPTIME", 768, NULL, tskIDLE_PRIORITY,
		&uptime_task);
}
