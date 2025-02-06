#pragma once

#include <stdatomic.h>

#include <esp_http_server.h>

typedef struct stats {
	_Atomic unsigned long uptime_seconds;
	
	_Atomic unsigned long mem_all_allocs;
	_Atomic unsigned long mem_all_frees;
	
	_Atomic unsigned long eth_recv_elap_frames;
	_Atomic unsigned long eth_recv_aarp_frames;
} stats_t;

// stats is the variable to stuff all our stats in.  It'll be exported
// to prometheus.
extern stats_t stats;

// Other gubbins
void start_stats_workers(void);
esp_err_t http_metrics_handler(httpd_req_t *req);