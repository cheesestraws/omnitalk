# Testing

## Writing tests

OmniTalk contains a rudimentary unit testing framework.  To create tests, create an X_test.c and X_test.h file anywhere in the codebase, for any X you like.  This test file needs to include "test.h".  Make sure the X_test.c is in the list of things to compile in the cmake file.  You can then use the TEST_FUNCTION macro to create a test function in both the .c and .h files.  Thus:

*X_test.h*:
```
#pragma once
#include "test.h"

TEST_FUNCTION(test_name);
```

*X_test.c*:
```
#include "X_text.h"

TEST_FUNCTION(test_name) {
	TEST_OK();
};
```

Use the TEST_OK() macro if your test passes; use TEST_FAIL() for fail; use TEST_ASSERT() to fail if a boolean expression is false.

## Running tests

Tests run on the OmniTalk hardware.  The list of tests to run is generated automatically by mktests.pl.  It makes a vague attempt to autodiscover all TEST_FUNCTIONs  This perl script is run automatically as part of the build.

Tests will run before the main body of the application if RUN_TESTS is defined in `tunables.h`.
