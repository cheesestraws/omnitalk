#pragma once

#include "net/tashtalk/state_machine.h"

extern tashtalk_rx_state_t* rxstate;
extern QueueHandle_t tashtalk_inbound_queue;
extern QueueHandle_t tashtalk_outbound_queue;

void tt_uart_init(void);
void tt_uart_start(void);