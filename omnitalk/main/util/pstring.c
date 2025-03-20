#include "util/pstring.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// uppercase according to appletalk rules
static char mac_uc(char c) {
	// ascii gubbins
	if (c >= 'a' && c <= 'z') {
		return c - 32;
	}
	
	switch (c) {
		case 0x88:
			return 0xCB;
		case 0x8A:
			return 0x80;
		case 0x8B:
			return 0xCC;
		case 0x8C:
			return 0x81;
		case 0x8D:
			return 0x82;
		case 0x8E:
			return 0x83;
		case 0x96:
			return 0x84;
		case 0x9A:
			return 0x85;
		case 0x9B:
			return 0xCD;
		case 0x9F:
			return 0x86;
		case 0xBE:
			return 0xAE;
		case 0xBF:
			return 0xAF;
		case 0xCF:
			return 0xCE;
		default:
			return c;
	}
}

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

bool pstring_eq_cstring_mac_ci(pstring *p, const char *c) {
	if (strlen(c) != p->length) {
		return false;
	}
	
	for (int i = 0; i < p->length; i++) {
		if (mac_uc(c[i]) != mac_uc(p->str[i])) {
			return false;
		}
	}
	
	return true;
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
