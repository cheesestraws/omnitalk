#pragma once

#include "net/transport.h"

#define B2UDPTUNNEL_QUEUE_DEPTH 60

void start_b2udptunnel(void);
transport_t* b2_get_transport(void);