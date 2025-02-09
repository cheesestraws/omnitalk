#pragma once

#include <esp_netif_types.h>

#define ETHERNET_FRAME_LEN 1522

extern esp_netif_t* _Atomic active_ip_net_if;

char *generate_hostname(void);