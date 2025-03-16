#pragma once

#include "table/zip/table_impl.h"

#include <stddef.h>

zt_zip_table_t* zt_new();

size_t zt_count_net_ranges(zt_zip_table_t *table);
bool zt_contains_net(zt_zip_table_t *table, uint16_t network);

bool zt_add_net_range(zt_zip_table_t *table, uint16_t net_start, uint16_t net_end);

// Delete a network range from the ZIP table.
bool zt_delete_network(zt_zip_table_t *table, uint16_t network);

bool zt_get_network_complete(zt_zip_table_t *table, uint16_t network);

// Add a zone for the given network.  Copies 'zone'.
void zt_add_zone_for(zt_zip_table_t *table, uint16_t network, char* zone);

void zt_set_expected_zone_count(zt_zip_table_t *table, uint16_t network, int count);
void zt_check_zone_count_for_completeness(zt_zip_table_t *table, uint16_t network);

void zt_mark_network_complete(zt_zip_table_t *table, uint16_t network);

size_t zt_count_zones_for(zt_zip_table_t *table, uint16_t network);
bool zt_network_is_complete(zt_zip_table_t *table, uint16_t network);

void zt_print(zt_zip_table_t *table);
