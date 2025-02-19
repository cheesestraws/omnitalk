#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "net/net.h"
#include "web/stats.h"
#include "web/web.h"
#include "controlplane_runloop.h"
#include "router_runloop.h"

void app_main(void)
{
	runloop_info_t controlplane;
	runloop_info_t router;

	printf("Welcome to OmniTalk\n");
	printf("Version: %s\n", GIT_VERSION);
	start_stats();

	start_net();	
	start_web();
	
	controlplane = start_controlplane_runloop();
	router = start_router_runloop();
	
	controlplane=controlplane;
	router = router;
	
	while(1) {
		vTaskDelay(portMAX_DELAY);
	}
}
