#include "net/tashtalk/uart.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/uart.h>
#include "driver/gpio.h"

#include "hw.h"
#include "mem/buffers.h"
#include "net/tashtalk/state_machine.h"
#include "web/stats.h"

static const char* TAG = "TT_UART";
static const int uart_num = UART_NUM_1;

TaskHandle_t uart_rx_task = NULL;
TaskHandle_t uart_tx_task = NULL;
QueueHandle_t uart_rx_queue = NULL;
QueueHandle_t uart_tx_queue = NULL;

#define RX_BUFFER_SIZE 1024
uint8_t uart_buffer[RX_BUFFER_SIZE];

void tt_uart_init_tashtalk(void) {
	// Sending 1k of zeroes is guaranteed to get tashtalk back into
	// a known state.
	char init[32] = { 0 };
	for (int i = 0; i < 32; i++) {
		uart_write_bytes(uart_num, init, 32);
	}
	
	// Then send an all-zero set of nodebits so we don't have an address.
	uart_write_bytes(uart_num, "\x02", 1);
	uart_write_bytes(uart_num, init, 32);
}

void tt_uart_init(void) {	
	const uart_config_t uart_config = {
		.baud_rate = 1000000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = 0,		
	};
	
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TX, UART_RX, UART_RTS, UART_CTS));
	ESP_ERROR_CHECK(uart_driver_install(uart_num, 2048, 0, 0, NULL, 0));

	tt_uart_init_tashtalk();

	ESP_LOGI(TAG, "tt_uart_init complete");
}

void tt_uart_rx_runloop(void* dummy) {
	static const char *TAG = "UART_RX";	
	ESP_LOGI(TAG, "started");
	
	tashtalk_rx_state_t* rxstate = new_tashtalk_rx_state(uart_rx_queue);
	
	while(1){
		/* TODO: this is stupid, read a byte at a time instead and wait for MAX_DELAY */
		const int len = uart_read_bytes(uart_num, uart_buffer, 1, 1000 / portTICK_PERIOD_MS);
		
		stats.tashtalk_raw_uart_in_octets += len;
		if (len > 0) {
			tashtalk_feed_all(rxstate, uart_buffer, len);
		}
	}
}

void tt_uart_start(void) {
	uart_rx_queue = xQueueCreate(60, sizeof(buffer_t*));;
	uart_tx_queue = xQueueCreate(60, sizeof(buffer_t*));;
	xTaskCreate(&tt_uart_rx_runloop, "UART_RX", 4096, NULL, 5, &uart_rx_task);
//	xTaskCreate(&uart_tx_runloop, "UART_TX", 4096, (void*)packet_pool, 5, &uart_tx_task);
}
