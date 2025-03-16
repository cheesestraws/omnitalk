#include "util/string.h"

#include <stdlib.h>
#include <string.h>

char* strclone(const char* src) {
	char* dst = malloc(strlen(src) + 1);
	if (dst == NULL) {
		return NULL;
	} else {
		strcpy(dst, src);
	}
	
	return dst;
}
