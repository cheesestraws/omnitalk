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
