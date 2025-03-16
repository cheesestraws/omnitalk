#include "util/pstring.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void pstring_print(pstring *p) {
	for (int i = 0; i < p->length; i++) {
		putchar((int)p->str[i]);
	}
}

bool pstring_eq_cstring(pstring *p, const char *c) {
	if (strlen(c) != p->length) {
		return false;
	}
	
	return memcmp(c, &p->str[0], (size_t)p->length) == 0;
}

char *pstring_to_cstring_alloc(pstring *p) {
	char *dst = malloc(p->length + 1);
	if (dst == NULL) {
		return NULL;
	}
	memcpy(dst, &p->str[0], p->length);
	dst[p->length] = 0;
	return dst;
}
