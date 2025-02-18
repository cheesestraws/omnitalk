#pragma once

#include "net/tashtalk/state_machine.h"

extern _Atomic bool tashtalk_enable_uart_tx;
extern tashtalk_rx_state_t* rxstate;
extern QueueHandle_t tashtalk_inbound_queue;
extern QueueHandle_t tashtalk_outbound_queue;

extern uint8_t tashtalk_node_address;

esp_err_t tt_uart_refresh_address(void);
void tt_uart_init(void);
void tt_uart_start(void);
