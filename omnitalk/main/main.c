#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "web/stats.h"
#include "controlplane_runloop.h"
#include "router_runloop.h"

void app_main(void)
{
	runloop_info_t controlplane;
	runloop_info_t router;

	printf("Welcome to OmniTalk\n");
	
	controlplane = start_controlplane_runloop();
	router = start_router_runloop();
	
	controlplane=controlplane;
	router = router;
	
	while(1) {
		vTaskDelay(portMAX_DELAY);
	}
}
