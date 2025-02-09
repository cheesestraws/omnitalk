#pragma once

#include <stdatomic.h>

#include <esp_http_server.h>

typedef _Atomic unsigned long prometheus_counter_t;
typedef _Atomic unsigned long prometheus_gauge_t;


typedef struct stats {
	prometheus_counter_t uptime_seconds;
	
	prometheus_counter_t mem_all_allocs;
	prometheus_counter_t mem_all_frees;
	prometheus_gauge_t mem_total_free_bytes__type_heap;
	prometheus_gauge_t mem_minimum_free_bytes__type_heap;
	prometheus_gauge_t mem_largest_free_block__type_heap;
	prometheus_gauge_t mem_total_free_bytes__type_dma;
	prometheus_gauge_t mem_minimum_free_bytes__type_dma;
	prometheus_gauge_t mem_largest_free_block__type_dma;
	
	prometheus_counter_t tashtalk_raw_uart_in_octets;
	prometheus_counter_t tashtalk_llap_rx_frame_count;
	prometheus_counter_t tashtalk_llap_too_long_count;
	prometheus_counter_t tashtalk_crc_fail_count; // rx
	prometheus_counter_t tashtalk_framing_error_count;
	prometheus_counter_t tashtalk_frame_abort_count;
	prometheus_counter_t tashtalk_inbound_path_queue_full;
	prometheus_counter_t tashtalk_err_rx_too_short_count;
	prometheus_counter_t tashtalk_err_tx_too_short_count;
	prometheus_counter_t tashtalk_err_tx_too_short_data;
	prometheus_counter_t tashtalk_err_tx_too_long_control;
	prometheus_counter_t tashtalk_err_tx_impossible_length;
	prometheus_counter_t tashtalk_err_tx_length_mismatch;
	prometheus_counter_t tashtalk_err_tx_crc_bad;
	prometheus_counter_t tashtalk_err_tx_no_room_for_crc;


	
	prometheus_counter_t eth_recv_elap_frames;
	prometheus_counter_t eth_recv_aarp_frames;
	prometheus_counter_t eth_input_path_ifInOctets;
	prometheus_counter_t eth_output_path_ifOutOctets;
	prometheus_counter_t eth_input_path_queue_full;

} stats_t;

// stats is the variable to stuff all our stats in.  It'll be exported
// to prometheus.
extern stats_t stats;

// Other gubbins
void start_stats_workers(void);
esp_err_t http_metrics_handler(httpd_req_t *req);