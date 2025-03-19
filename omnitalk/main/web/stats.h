#pragma once

#include <stdatomic.h>
#include <stdint.h>

#include <esp_http_server.h>

#include "lap/id.h"

#define PROMETHEUS_METADATA

typedef _Atomic unsigned long prometheus_counter_t;
typedef _Atomic unsigned long prometheus_gauge_t;

typedef PROMETHEUS_METADATA struct stats_omnitalk_metadata_s {
	_Atomic bool ok;
	char* git_commit;
	const char* esp_version;
} stats_omnitalk_metadata_t;

typedef PROMETHEUS_METADATA struct stats_lap_metadata_s {
	_Atomic bool ok;
	_Atomic(char*) name;
	_Atomic(char*) state;
	_Atomic(char*) zone;
	_Atomic uint8_t node_address;
	_Atomic uint16_t discovered_network;
} stats_lap_metadata_t;

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
	   transport_in_errors{transport="localtalk",err="wrong hat"}
	   transport_out_errors{transport="localtalk",err="bad smell"}
	*/
	
	prometheus_counter_t transport_in_octets__transport_localtalk;
	prometheus_counter_t transport_out_octets__transport_localtalk; 
	prometheus_counter_t transport_in_frames__transport_localtalk;
	prometheus_counter_t transport_out_frames__transport_localtalk;
	prometheus_counter_t transport_in_errors__transport_localtalk__err_frame_too_long; 
	prometheus_counter_t transport_in_errors__transport_localtalk__err_frame_too_short;
	prometheus_counter_t transport_in_errors__transport_localtalk__err_bad_crc;
	prometheus_counter_t transport_in_errors__transport_localtalk__err_framing_error;
	prometheus_counter_t transport_in_errors__transport_localtalk__err_frame_abort;
	prometheus_counter_t transport_in_errors__transport_localtalk__err_lap_queue_full;
	prometheus_counter_t tashtalk_rx_control_packets_not_forwarded;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_packet_too_short;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_data_packet_too_short;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_control_packet_too_long;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_packet_length_impossible;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_packet_length_inconsistent;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_bad_crc;
	prometheus_counter_t transport_out_errors__transport_localtalk__err_no_room_for_crc_in_buffer;

	prometheus_counter_t transport_in_octets__transport_ltoudp;
	prometheus_counter_t transport_out_octets__transport_ltoudp; 
	prometheus_counter_t transport_in_frames__transport_ltoudp;
	prometheus_counter_t transport_out_frames__transport_ltoudp;
	prometheus_counter_t transport_in_errors__transport_ltoudp__err_recv_failed;
	prometheus_counter_t transport_in_errors__transport_ltoudp__err_packet_too_long;
	prometheus_counter_t transport_in_errors__transport_ltoudp__err_lap_queue_full;
	prometheus_counter_t transport_out_errors__transport_ltoudp__err_send_failed;
	
	prometheus_counter_t transport_in_octets__transport_b2udp;
	prometheus_counter_t transport_out_octets__transport_b2udp;
	prometheus_counter_t transport_in_frames__transport_b2udp;
	prometheus_counter_t transport_out_frames__transport_b2udp;
	prometheus_counter_t transport_in_errors__transport_b2udp__err_recvfrom_failed;
	prometheus_counter_t transport_in_errors__transport_b2udp__err_frame_too_short;
	prometheus_counter_t transport_in_errors__transport_b2udp__err_invalid_source_MAC;
	prometheus_counter_t transport_in_errors__transport_b2udp__err_lap_queue_full;
	prometheus_counter_t transport_out_errors__transport_b2udp__err_frame_too_short;
	prometheus_counter_t transport_out_errors__transport_b2udp__err_invalid_dst_MAC;
	prometheus_counter_t transport_out_errors__transport_b2udp__err_sendto_failed;

	prometheus_counter_t transport_in_octets__transport_ethernet;
	prometheus_counter_t transport_out_octets__transport_ethernet;
	prometheus_counter_t transport_in_frames__transport_ethernet;
	prometheus_counter_t transport_out_frames__transport_ethernet;
	prometheus_counter_t eth_recv_elap_frames; // help: ethernet: received ELAP frames (raw count)
	prometheus_counter_t eth_recv_aarp_frames; // help: ethernet: received AARP frames (raw count)
	prometheus_counter_t transport_in_errors__transport_ethernet__err_lap_queue_full;
	
	// LAP registry
	prometheus_counter_t lap_registry_registered_laps;
	
	// DDP metrics
	prometheus_counter_t ddp_out_errors__err_no_route_for_network;
	
	// Control plane metrics
	prometheus_counter_t controlplane_inbound_queue_full;
	
	// RTMP
	prometheus_counter_t rtmp_update_packets;
	prometheus_counter_t rtmp_errors__err_packet_too_short;
	prometheus_counter_t rtmp_errors__err_wrong_id_len;
	
	// ZIP
	prometheus_counter_t zip_out_queries;
	prometheus_counter_t zip_out_errors__err_ddp_send_failed;
	prometheus_counter_t zip_in_replies__kind_nonextended;
	prometheus_counter_t zip_in_replies__kind_extended;
} stats_t;

// stats is the variable to stuff all our stats in.  It'll be exported
// to prometheus.
extern stats_t stats;

extern PROMETHEUS_METADATA stats_omnitalk_metadata_t stats_omnitalk_metadata;
extern PROMETHEUS_METADATA stats_lap_metadata_t stats_lap_metadata[MAX_LAP_COUNT];

// routing table metrics
extern _Atomic(char*) stats_routing_table;

// zip table metrics
extern _Atomic(char*) stats_zip_table;


// Other gubbins
void start_stats(void);
esp_err_t http_metrics_handler(httpd_req_t *req);
