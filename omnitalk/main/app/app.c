#include "app/app.h"

#include "app/aep/aep.h"
#include "app/nbp/nbp.h"
#include "app/rtmp/rtmp.h"
#include "app/sip/sip.h"
#include "app/zip/zip.h"
#include "net/common.h"

app_t unicast_apps[] = {
	{ .socket_number = 1, .name = "app/rtmp", .handler = &app_rtmp_handler, .idle = &app_rtmp_idle, .start = &app_rtmp_start },
	{ .socket_number = 2, .name = "app/nbp", .handler = &app_nbp_handler, .start = &app_nbp_start },
	{ .socket_number = 4, .handler = &app_aep_handler, .nbp_object_is_hostname = true, .nbp_type = "Workstation" },
	{ .socket_number = 6, .name = "app/zip", .handler = &app_zip_handler, .idle = &app_zip_idle, .start = &app_zip_start },
	
	{ .socket_number = 253, .name = "app/sip", .handler = &app_sip_handler, .nbp_object_is_hostname = true, .nbp_type = "OmniTalk" },
	
	{ .socket_number = 0, .handler = NULL }
};

static char* my_hostname;

void start_apps(void) {
	my_hostname = generate_hostname();
	for (int i = 0; unicast_apps[i].socket_number != 0; i++) {
		// Do any apps want the hostname in their object?
		if (unicast_apps[i].nbp_object_is_hostname) {
			unicast_apps[i].nbp_object = my_hostname;
		}
		if (unicast_apps[i].start != NULL) {
			unicast_apps[i].start();
		}
		if (unicast_apps[i].idle != NULL) {
			xTaskCreate(unicast_apps[i].idle, unicast_apps[i].name, 4096, NULL, tskIDLE_PRIORITY, NULL);
		}
	}
}
