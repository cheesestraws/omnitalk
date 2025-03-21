#include "app/app.h"

#include "app/aep/aep.h"
#include "app/nbp/nbp.h"
#include "app/rtmp/rtmp.h"
#include "app/zip/zip.h"

app_t unicast_apps[] = {
	{ .socket_number = 1, .name = "app/rtmp", .handler = &app_rtmp_handler, .idle = &app_rtmp_idle, .start = &app_rtmp_start },
	{ .socket_number = 2, .name = "app/nbp", .handler = &app_nbp_handler, .start = &app_nbp_start },
	{ .socket_number = 4, .handler = &app_aep_handler },
	{ .socket_number = 6, .name = "app/zip", .handler = &app_zip_handler, .idle = &app_zip_idle, .start = &app_zip_start },
	{ .socket_number = 0, .handler = NULL }
};

void start_apps(void) {
	for (int i = 0; unicast_apps[i].socket_number != 0; i++) {
		if (unicast_apps[i].start != NULL) {
			unicast_apps[i].start();
		}
		if (unicast_apps[i].idle != NULL) {
			xTaskCreate(unicast_apps[i].idle, unicast_apps[i].name, 4096, NULL, tskIDLE_PRIORITY, NULL);
		}
	}
}
