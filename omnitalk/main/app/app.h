#pragma once

#include "mem/buffers.h"

typedef void(*app_packet_handler)(buffer_t*);
typedef void(*app_idle_task)(void*);
typedef void(*app_start)(void);



typedef struct {
	uint8_t socket_number;
	char* name;
	app_packet_handler handler;
	app_idle_task idle; 
	app_start start;
	
	char* nbp_object;
	bool nbp_object_is_hostname;
	char* nbp_type;
} app_t;

extern app_t unicast_apps[];

void start_apps(void);
