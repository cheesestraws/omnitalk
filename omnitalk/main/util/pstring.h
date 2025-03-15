#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint8_t length;
	char str[];
} pstring;

void pstring_print(pstring *p);
bool pstring_eq_cstring(pstring *p, const char *c);
