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

void update_memory_stats() {
	multi_heap_info_t heap_info;

	// heap stats
	heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);
	stats.mem_total_free_bytes__type_heap = heap_info.total_free_bytes;
	stats.mem_minimum_free_bytes__type_heap = heap_info.minimum_free_bytes;
	stats.mem_largest_free_block__type_heap = heap_info.largest_free_block;
	
	// DMA stats
	heap_caps_get_info(&heap_info, MALLOC_CAP_DMA);
	stats.mem_total_free_bytes__type_dma = heap_info.total_free_bytes;
	stats.mem_minimum_free_bytes__type_dma = heap_info.minimum_free_bytes;
	stats.mem_largest_free_block__type_dma = heap_info.largest_free_block;

}