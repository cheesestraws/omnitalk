#include "app/app.h"

#include "app/aep/aep.h"
#include "app/rtmp/rtmp.h"

app_t unicast_apps[] = {
	{ .socket_number = 1, .handler = &app_rtmp_handler },
	{ .socket_number = 4, .handler = &app_aep_handler },
	{ .socket_number = 0, .handler = NULL }
};
