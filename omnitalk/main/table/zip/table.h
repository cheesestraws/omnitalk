#pragma once

#include "table/zip/table_impl.h"

#include <stdbool.h>
#include <stddef.h>

#include "util/pstring.h"

zt_zip_table_t* zt_new();

size_t zt_count_net_ranges(zt_zip_table_t *table);
bool zt_contains_net(zt_zip_table_t *table, uint16_t network);

bool zt_add_net_range(zt_zip_table_t *table, uint16_t net_start, uint16_t net_end);

// Delete a network range from the ZIP table.
bool zt_delete_network(zt_zip_table_t *table, uint16_t network);

bool zt_get_network_complete(zt_zip_table_t *table, uint16_t network);

// Add a zone for the given network.  Copies 'zone'.
void zt_add_zone_for(zt_zip_table_t *table, uint16_t network, pstring* zone);

void zt_set_expected_zone_count(zt_zip_table_t *table, uint16_t network, int count);
void zt_check_zone_count_for_completeness(zt_zip_table_t *table, uint16_t network);

void zt_mark_network_complete(zt_zip_table_t *table, uint16_t network);

size_t zt_count_zones_for(zt_zip_table_t *table, uint16_t network);
bool zt_network_is_complete(zt_zip_table_t *table, uint16_t network);

void zt_print(zt_zip_table_t *table);
char* zt_stats(zt_zip_table_t *table);

// An iterator.  We can't do the for-loop-and-cursor trick, because we need to iterate
// under the lock.  So instead we do it with callbacks.  If any callback returns false,
// the iteration terminates (calling the finaliser if one is provided.)
typedef bool (*zip_iterator_init_cb)(void* pvt, uint16_t network, bool exists, size_t zone_count, bool complete);
// In the loop body iterator, do not modify the zone char* you are given.  If you want
// to copy it, do so before the callback returns; do not stash the pointer and attempt to
// copy it later.
typedef bool (*zip_iterator_loop_cb)(void* pvt, int idx, uint16_t network, pstring* zone);
typedef bool (*zip_iterator_end_cb)(void* pvt, bool aborted);

bool zt_iterate_net(
	zt_zip_table_t *table, void* private_data, uint16_t network,
	zip_iterator_init_cb init,
	zip_iterator_loop_cb,
	zip_iterator_end_cb);

// Iterator for zone names.  This is highly fragile, because of course indexes don't
// stay still when routers are joining and leaving.  But the GetZoneList/GetLocalZoneList
// ZIP protocol operations are defined in terms of indexes, so we have to support querying
// by them.
//
// This API is intentionally hobbled to prevent it being used for anything else.
// Return false from the callback to bail out.  If you get a NULL pstring, that means
// you've reached the end of the list.
typedef bool(*zip_zone_name_iterator)(void* pvt, pstring* zone);

bool zt_iterate_zone_names(zt_zip_table_t *table, void* private_data,
	int starting_index, zip_zone_name_iterator callback);
	
bool zt_iterate_zone_names_for_net(zt_zip_table_t *table, void* private_data,
	uint16_t network, int starting_index, zip_zone_name_iterator callback);

