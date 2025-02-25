#pragma once

#include "runloop.h"

#define CONTROLPLANE_QUEUE_DEPTH 60

runloop_info_t start_controlplane_runloop(void);
