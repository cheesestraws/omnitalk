#include "util/macroman.h"

#include <stddef.h>
#include <string.h>

#include "util/pstring.h"

static size_t num_corresponding_utf8_chars(unsigned char* ptr, size_t buf_size) {
	size_t num = 0;
	for (size_t i = 0; i < buf_size; i++) {
		num += macroman_to_utf8_lengths[(int)ptr[i]];
	}
	return num;
}

size_t pstring_macroman_to_utf8_length(pstring* str) {
	return num_corresponding_utf8_chars((unsigned char*)str->str, (size_t)str->length);
}

size_t cstring_macroman_to_utf8_length(char* str) {
	return num_corresponding_utf8_chars((unsigned char*)str, strlen(str));
}

char* pstring_to_cstring_and_macroman_to_utf8_alloc(pstring* str) {
	size_t len = pstring_macroman_to_utf8_length(str);
	char *dst = malloc(len + 1);
	if (dst == NULL) {
		return NULL;
	}
	
	char* cursor = dst;
	for (int i = 0; i < str->length; i++) {
		cursor = stpcpy(cursor, macroman_to_utf8[(int)str->str[i]]);
	}
	
	dst[len] = 0;
	return dst;
}
