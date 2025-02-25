#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct runloop_info {
	TaskHandle_t task;
	QueueHandle_t incoming_packet_queue;
} runloop_info_t;
