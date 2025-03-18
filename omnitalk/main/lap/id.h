#pragma once

#include <freertos/FreeRTOSConfig.h>

// Look, don't blame me, blame FreeRTOS.  Quoth the manual:
// "The number of bits (or flags) implemented within an event group is 8 if 
// configUSE_16_BIT_TICKS is set to 1, or 24 if configUSE_16_BIT_TICKS is set to 0."

#if configUSE_16_BIT_TICKS == 0
#define MAX_LAP_COUNT 24
#elif configUSE_16_BIT_TICKS == 1
#define MAX_LAP_COUNT 8
#else
#error what have you done to configUSE_16_BIT_TICKS?
#endif

// gets a unique ID for a LAP.
int get_next_lap_id(void);
