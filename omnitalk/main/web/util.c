#include "web/util.h"

#include <stddef.h>

void urlndecode(char *dst, const char *src, size_t len) {
	char a, b;
	size_t cnt = 0;
	while (*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(isxdigit(a) && isxdigit(b))) {
				if (a >= 'a')
						a -= 'a'-'A';
				if (a >= 'A')
						a -= ('A' - 10);
				else
						a -= '0';
				if (b >= 'a')
						b -= 'a'-'A';
				if (b >= 'A')
						b -= ('A' - 10);
				else
						b -= '0';
				*dst++ = 16*a+b;
				src+=3;
		} else if (*src == '+') {
				*dst++ = ' ';
				src++;
		} else {
				*dst++ = *src++;
		}
		cnt++;
		if (cnt >= (len - 1)) {
			break;
		}
	}
	*dst++ = '\0';
}