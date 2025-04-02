#pragma once

#include <stdint.h>

#include "table/routing/route.h"

typedef enum {
	ZIP_NETWORK_TOUCHED,
	ZIP_NETWORK_DELETED
} zip_internal_command_type_t;

typedef struct {
	zip_internal_command_type_t cmd;
	rt_route_t route;
} zip_internal_command_t;

void app_zip_handle_get_zone_list(buffer_t *packet);
void app_zip_handle_get_net_info(buffer_t *packet);
void app_zip_handle_query(buffer_t *packet);
