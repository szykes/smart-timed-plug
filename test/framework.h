#ifndef TEST_FRAMEWORK_H_
#define TEST_FRAMEWORK_H_

#include <stdbool.h>
#include "mock.h"

#define log_error(format, ...) own_log(__func__, __LINE__, "ERROR", format, ##__VA_ARGS__)
#define log_fail(format, ...) own_log(__func__, __LINE__, "FAIL", format, ##__VA_ARGS__)
#define log_info(format, ...) own_log(__func__, __LINE__, "INFO", format, ##__VA_ARGS__)
#define log_test(format, ...) own_log("", 0, "TEST", format, ##__VA_ARGS__)

void own_log(const char *func, unsigned int line, const char *lvl, const char *format, ...);

#define TEST_BEGIN() \
  log_test("--------------------------------------------------------"); \
  log_test("TC: %s", __func__);						\
  bool is_succeeded = true;						\
  mock_clear_calls();

#define TEST_ASSERT_EQ(actual, expected, msg)				\
  if (actual != expected) {						\
    log_fail("not equal %s: actual: %d, expected: %d", msg, actual, expected); \
    is_succeeded = false;						\
  }

#define TEST_ASSERT_BOOL(value)						\
  if (!value) {								\
    log_fail("not true");						\
    is_succeeded = false;						\
  }

#define TEST_END()							\
  bool is_mock_succeeded = mock_is_succeeded();				\
  if (is_succeeded && is_mock_succeeded) {				\
    log_test("Test succeeded");						\
    return true;							\
  }									\
  log_fail("Test FAILED!!!!!");                                         \
  exit(1);


#define TEST_EVALUATE_INIT()						\
  bool is_succeeded = true;						\
  mock_init();

#define TEST_EVALUATE(value)						\
  if (!value) {								\
    is_succeeded = false;						\
  }

#define TEST_EVALUATE_END()						\
  log_test("--------------------------------------------------------"); \
  if (is_succeeded) {							\
    log_test("SUM: ALL Tests succeeded");				\
    return 0;								\
  }									\
  log_fail("SUM: Some Test(s) FAILED!!!!!");				\
  return 1;


#endif // TEST_FRAMEWORK_H_
