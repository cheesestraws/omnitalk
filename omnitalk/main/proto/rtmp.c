#include "proto/rtmp.h"

#include <stdio.h>

void print_rtmp_tuple(rtmp_tuple_t *tuple) {
	if (tuple == NULL) {
		printf("    (null)\n");
		return;
	}
		
	if (!RTMP_TUPLE_IS_EXTENDED(tuple)) {
		printf("    net %d distance %d\n",
			(int)(RTMP_TUPLE_NETWORK(tuple)),
			(int)(RTMP_TUPLE_DISTANCE(tuple)));
	} else {
		printf("    net range %d - %d distance %d\n",
			(int)(RTMP_TUPLE_RANGE_START(tuple)),
			(int)(RTMP_TUPLE_RANGE_END(tuple)),
			(int)(RTMP_TUPLE_DISTANCE(tuple)));
	}
}
