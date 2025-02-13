#pragma once

#include <stdatomic.h>

#include <esp_http_server.h>

typedef _Atomic unsigned long prometheus_counter_t;
typedef _Atomic unsigned long prometheus_gauge_t;


typedef struct stats {
	prometheus_counter_t uptime_seconds; // help: system uptime in seconds
	
	prometheus_counter_t mem_all_allocs; // help: all heap allocations
	prometheus_counter_t mem_all_frees; // help: all heap frees
	prometheus_gauge_t mem_total_free_bytes__type_heap; // help: total free bytes
	prometheus_gauge_t mem_minimum_free_bytes__type_heap; // help: minimum free bytes we've seen so far
	prometheus_gauge_t mem_largest_free_block__type_heap; // help: largest available free block
	prometheus_gauge_t mem_total_free_bytes__type_dma; // help: total free bytes
	prometheus_gauge_t mem_minimum_free_bytes__type_dma; // help: minimum free bytes we've seen so far
	prometheus_gauge_t mem_largest_free_block__type_dma; // help: largest available free block
	
	/* Transport metrics should look like:
	   transport_in_octets{transport="localtalk"}
	   transport_out_octets{transport="localtalk"}
	   transport_in_frames{transport="localtalk"}
	   transport_out_frames{transport="localtalk"}
	*/
	
	prometheus_counter_t transport_in_octets__transport_localtalk;
	prometheus_counter_t transport_out_octets__transport_localtalk; 
	prometheus_counter_t transport_in_frames__transport_localtalk;
	prometheus_counter_t transport_out_frames__transport_localtalk;
	prometheus_counter_t tashtalk_llap_too_long_count; // help: tashtalk: llap packet overflows
	prometheus_counter_t tashtalk_crc_fail_count; // help: tashtalk: llap rx crc failures
	prometheus_counter_t tashtalk_framing_error_count; // help: tashtalk: llap rx frame errors
	prometheus_counter_t tashtalk_frame_abort_count; // help: tashtalk: llap rx frame aborts
	prometheus_counter_t tashtalk_inbound_path_queue_full; //help: tashtalk: rx could not write frame to queue
	prometheus_counter_t tashtalk_rx_control_packets_not_forwarded; // help: tashtalk: rx packets discarded because they're control packets
	prometheus_counter_t tashtalk_err_rx_too_short_count;
	prometheus_counter_t tashtalk_err_tx_too_short_count;
	prometheus_counter_t tashtalk_err_tx_too_short_data;
	prometheus_counter_t tashtalk_err_tx_too_long_control;
	prometheus_counter_t tashtalk_err_tx_impossible_length;
	prometheus_counter_t tashtalk_err_tx_length_mismatch;
	prometheus_counter_t tashtalk_err_tx_crc_bad;
	prometheus_counter_t tashtalk_err_tx_no_room_for_crc;

	prometheus_counter_t transport_in_octets__transport_ltoudp;
	prometheus_counter_t transport_out_octets__transport_ltoudp; 
	prometheus_counter_t transport_in_frames__transport_ltoudp;
	prometheus_counter_t transport_out_frames__transport_ltoudp;
	prometheus_counter_t ltoudp_err_rx_recv_error;
	prometheus_counter_t ltoudp_err_rx_packet_too_long;
	prometheus_counter_t ltoudp_err_rx_queue_full;
	prometheus_counter_t ltoudp_err_tx_send_error;
	
	prometheus_counter_t transport_in_octets__transport_b2udp;
	prometheus_counter_t transport_out_octets__transport_b2udp;
	prometheus_counter_t transport_in_frames__transport_b2udp;
	prometheus_counter_t transport_out_frames__transport_b2udp;
	prometheus_counter_t b2eth_err_rx_recvfrom_failed;
	prometheus_counter_t b2eth_err_rx_frame_too_short;
	prometheus_counter_t b2eth_err_rx_invalid_src_mac;
	prometheus_counter_t b2eth_err_tx_frame_too_short;
	prometheus_counter_t b2eth_err_tx_invalid_dst_mac;
	prometheus_counter_t b2eth_err_tx_sendto_failed;

	prometheus_counter_t transport_in_octets__transport_ethernet;
	prometheus_counter_t transport_out_octets__transport_ethernet;
	prometheus_counter_t transport_in_frames__transport_ethernet;
	prometheus_counter_t transport_out_frames__transport_ethernet;
	prometheus_counter_t eth_recv_elap_frames; // help: ethernet: received ELAP frames (raw count)
	prometheus_counter_t eth_recv_aarp_frames; // help: ethernet: received AARP frames (raw count)
	prometheus_counter_t eth_input_path_queue_full;
} stats_t;

// stats is the variable to stuff all our stats in.  It'll be exported
// to prometheus.
extern stats_t stats;

// Other gubbins
void start_stats_workers(void);
esp_err_t http_metrics_handler(httpd_req_t *req);