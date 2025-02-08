#include "net/tashtalk/uart.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/uart.h>
#include "driver/gpio.h"

#include "mem/buffers.h"
#include "net/common.h"
#include "net/tashtalk/state_machine.h"
#include "web/stats.h"
#include "hw.h"

static const char* TAG = "TT_UART";
static const int uart_num = UART_NUM_1;

TaskHandle_t uart_rx_task = NULL;
TaskHandle_t uart_tx_task = NULL;
QueueHandle_t tashtalk_inbound_queue = NULL;
QueueHandle_t tashtalk_outbound_queue = NULL;
tashtalk_rx_state_t* rxstate = NULL;

_Atomic bool tashtalk_enable_uart_tx = false;

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
	
	rxstate = new_tashtalk_rx_state(tashtalk_inbound_queue);
	
	while(1){
		/* TODO: this is stupid, read a byte at a time instead and wait for MAX_DELAY */
		const int len = uart_read_bytes(uart_num, uart_buffer, 1, 1000 / portTICK_PERIOD_MS);
		
		stats.tashtalk_raw_uart_in_octets += len;
		if (len > 0) {
			tashtalk_feed_all(rxstate, uart_buffer, len);
		}
	}
}

bool tashtalk_tx_validate(buffer_t* packet) {
	if (packet->length < 5) {
		// Too short
		ESP_LOGE(TAG, "tx packet too short");
		stats.tashtalk_err_tx_too_short_count++;
		return false;
	}
	
	if (packet->length == 5 && !(packet->data[2] & 0x80)) {
		// 3 byte packet is not a control packet
		ESP_LOGE(TAG, "tx 3-byte non-control packet, wut?");
		stats.tashtalk_err_tx_too_short_data++;
		return false;
	}
	
	if ((packet->data[2] & 0x80) && packet->length != 5) {
		// too long control frame
		ESP_LOGE(TAG, "tx too-long control packet, wut?");
		stats.tashtalk_err_tx_too_long_control++;
		return false;
	}
	
	if (packet->length == 6) {
		// impossible packet length
		ESP_LOGE(TAG, "tx impossible packet length, wut?");
		stats.tashtalk_err_tx_impossible_length++;
		return false;
	}
	
	if (packet->length >= 7 && (((packet->data[3] & 0x3) << 8) | packet->data[4]) != packet->length - 5) {
		// packet length does not match claimed length
		ESP_LOGE(TAG, "tx length field (%d) does not match actual packet length (%d)", (((packet->data[3] & 0x3) << 8) | packet->data[4]), packet->length - 5);
		stats.tashtalk_err_tx_length_mismatch++;
		return false;
	}

	// check the CRC
	crc_state_t crc;
	crc_state_init(&crc);
	crc_state_append_all(&crc, packet->data, packet->length);
	if (!crc_state_ok(&crc)) {
		ESP_LOGE(TAG, "bad CRC on tx: IP bug?");
		stats.tashtalk_err_tx_crc_bad++;
		return false;
	}
	
	return true;
}

void tt_uart_tx_runloop(void* buffer_pool) {
	buffer_t* packet = NULL;
	static const char *TAG = "UART_TX";
	
	ESP_LOGI(TAG, "started");
	
	while(1){
		xQueueReceive(tashtalk_outbound_queue, &packet, portMAX_DELAY);
		
		if (!tashtalk_tx_validate(packet)) {
			ESP_LOGE(TAG, "packet validation failed");
			goto skip_processing;
		}
		
		if (tashtalk_enable_uart_tx) {
			uart_write_bytes(uart_num, "\x01", 1);
			uart_write_bytes(uart_num, (const char*)packet->data, packet->length);
		}
skip_processing:
		freebuf(packet);
	}
}


void tt_uart_start(void) {
	tashtalk_inbound_queue = xQueueCreate(60, sizeof(buffer_t*));;
	tashtalk_outbound_queue = xQueueCreate(60, sizeof(buffer_t*));;
	xTaskCreate(&tt_uart_rx_runloop, "UART_RX", 4096, NULL, 20, &uart_rx_task);
	xTaskCreate(&tt_uart_tx_runloop, "UART_TX", 4096, NULL, 20, &uart_tx_task);
}
