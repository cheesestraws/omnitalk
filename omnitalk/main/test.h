#pragma once

// A hack to make sure we return
typedef int test_must_ok_or_fail_t;

#define TEST_FUNCTION(X) test_must_ok_or_fail_t X(char* _test_name)
#define RUN_TEST(X) X(#X)
#define TEST_OK() real_test_ok(_test_name); return 0
#define TEST_FAIL(MSG) real_test_fail(_test_name, MSG); return 0

TEST_FUNCTION(test_tests);

void real_test_ok(char*);
void real_test_fail(char*, char*);
void test_main(void);
