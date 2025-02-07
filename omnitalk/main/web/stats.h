#pragma once

#include <stdatomic.h>

#include <esp_http_server.h>

typedef struct stats {
	_Atomic unsigned long uptime_seconds;
	
	_Atomic unsigned long mem_all_allocs;
	_Atomic unsigned long mem_all_frees;
	_Atomic unsigned long mem_total_free_bytes__type_heap;
	_Atomic unsigned long mem_minimum_free_bytes__type_heap;
	_Atomic unsigned long mem_largest_free_block__type_heap;
	_Atomic unsigned long mem_total_free_bytes__type_dma;
	_Atomic unsigned long mem_minimum_free_bytes__type_dma;
	_Atomic unsigned long mem_largest_free_block__type_dma;
	
	_Atomic unsigned long tashtalk_raw_uart_in_octets;
	_Atomic unsigned long tashtalk_llap_rx_frame_count;
	_Atomic unsigned long tashtalk_llap_too_long_count;
	_Atomic unsigned long tashtalk_crc_fail_count;
	_Atomic unsigned long tashtalk_framing_error_count;
	_Atomic unsigned long tashtalk_frame_abort_count;
	
	_Atomic unsigned long eth_recv_elap_frames;
	_Atomic unsigned long eth_recv_aarp_frames;
	_Atomic unsigned long eth_input_path_ifInOctets;
	_Atomic unsigned long eth_output_path_ifOutOctets;

} stats_t;

// stats is the variable to stuff all our stats in.  It'll be exported
// to prometheus.
extern stats_t stats;

// Other gubbins
void start_stats_workers(void);
esp_err_t http_metrics_handler(httpd_req_t *req);