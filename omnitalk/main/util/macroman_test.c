#include "util/macroman_test.h"
#include "util/macroman.h"
#include "test.h"

#include <stdio.h>
#include <string.h>

TEST_FUNCTION(test_macroman_to_utf8) {
	// Lengths	
	TEST_ASSERT(cstring_macroman_to_utf8_length("\x01\x02\x03""ab") == 11);
	TEST_ASSERT(pstring_macroman_to_utf8_length((pstring*)"\x06""deffed") == 6);

	// Convert to cstring and change charset in one go?
	char* str;
	
	str = pstring_to_cstring_and_macroman_to_utf8_alloc((pstring*)"\x06""deffed");
	TEST_ASSERT(strcmp(str, "deffed") == 0);
	free(str);
	
	str = pstring_to_cstring_and_macroman_to_utf8_alloc((pstring*)"\x04\x01\x02\x03\xff");
	TEST_ASSERT(strcmp(str, "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xcb\x87") == 0);
	free(str);

	TEST_OK();
}
