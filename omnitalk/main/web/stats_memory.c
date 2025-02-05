#include <esp_heap_caps.h>

#include "web/stats.h"

#ifndef CONFIG_HEAP_USE_HOOKS
#error Please enable CONFIG_HEAP_USE_HOOKS
#endif

void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps) {
	stats.mem_all_allocs++;
}

void esp_heap_trace_free_hook(void* ptr) {
	stats.mem_all_frees++;
}