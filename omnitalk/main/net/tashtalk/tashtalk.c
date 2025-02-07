#include "net/tashtalk/tashtalk.h"

#include "net/tashtalk/uart.h"

void start_tashtalk(void) {
	tt_uart_init();
	tt_uart_start();
}