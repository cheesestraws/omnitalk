#pragma once

#include "mem/buffers.h"

void app_rtmp_handler(buffer_t *packet);
void app_rtmp_idle(void*);
void app_rtmp_start(void);
