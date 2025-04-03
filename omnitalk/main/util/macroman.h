#pragma once

#include <stddef.h>

#include "util/pstring.h"

extern char* macroman_to_utf8[];
extern size_t macroman_to_utf8_lengths[];

size_t pstring_macroman_to_utf8_length(pstring* str);
size_t cstring_macroman_to_utf8_length(char* str);

char* pstring_to_cstring_and_macroman_to_utf8_alloc(pstring* str);
