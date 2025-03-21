#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NBP_PARTIAL_WILDCARD '\xc5'
#define NBP_PARTIAL_WILDCARD_STR "\xc5"

typedef struct {
	uint8_t length;
	char str[];
} pstring;

void pstring_print(pstring *p);
int pstring_index_of(pstring *p, char c);
bool pstring_eq_cstring(pstring *p, const char *c);
bool pstring_eq_cstring_mac_ci(pstring *p, const char *c);
bool pstring_matches_cstring_nbp(pstring *p, const char *c);
char *pstring_to_cstring_alloc(pstring *p);
