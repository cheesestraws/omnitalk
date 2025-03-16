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
