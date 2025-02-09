#pragma once

#include "net/transport.h"

#define LTOUDP_QUEUE_DEPTH 60

void start_ltoudp(void);
transport_t* ltoudp_get_transport(void);