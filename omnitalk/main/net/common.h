#pragma once

#include <stdatomic.h>

#include <esp_netif_types.h>


#define ETHERNET_FRAME_LEN 1522

extern esp_netif_t* _Atomic active_ip_net_if;

char *generate_hostname(void);
void wait_for_ip_ready(void);
void mark_ip_ready(void);
void start_common(void);
