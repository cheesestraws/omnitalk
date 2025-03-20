#include "util/pstring_test.h"
#include "util/pstring.h"

#include <stdlib.h>

#include "test.h"

TEST_FUNCTION(test_pstring_round_tripping) {
	char *test_string = "\x0bhello world";
	pstring *test_pstring = (pstring*)test_string;
	
	char *result = pstring_to_cstring_alloc(test_pstring);
	TEST_ASSERT(pstring_eq_cstring(test_pstring, result));
	TEST_ASSERT(pstring_eq_cstring(test_pstring, "hello world"));
	
	free(result);
	
	TEST_OK();
}

TEST_FUNCTION(test_pstring_case_mapping) {
	pstring *test_pstring = (pstring*)"\x0bHeLlO wOrLd";
	TEST_ASSERT(!pstring_eq_cstring(test_pstring, "hello world"));
	TEST_ASSERT(pstring_eq_cstring_mac_ci(test_pstring, "hello world"));

	pstring *another_test_string = (pstring*)"\x27""abcdefghijklmnopqrstuvwxyz\x88\x8A\x8B\x8C\x8D\x8E\x96\x9A\x9B\x9F\xBE\xBF\xCF";
	TEST_ASSERT(!pstring_eq_cstring(another_test_string, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\xCB\x80\xCC\x81\x82\x83\x84\x85\xCD\x86\xAE\xAF\xCE"));
	TEST_ASSERT(pstring_eq_cstring_mac_ci(another_test_string, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\xCB\x80\xCC\x81\x82\x83\x84\x85\xCD\x86\xAE\xAF\xCE"));
	
	TEST_OK();
}
