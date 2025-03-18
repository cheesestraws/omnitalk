#pragma once

#include "lap/registry.h"
#include "table/routing/table.h"
#include "table/zip/table.h"

// Yeah, yeah, global state is icky, but it's easier to have a single routing table
// here.
extern rt_routing_table_t *global_routing_table;
extern zt_zip_table_t *global_zip_table;
extern lap_registry_t *global_lap_registry;
