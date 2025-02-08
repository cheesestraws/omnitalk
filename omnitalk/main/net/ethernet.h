#pragma once

#include "net/transport.h"

#define ETHERNET_QUEUE_DEPTH 60

void start_ethernet(void);
transport_t* ethertalkv2_get_transport(void);