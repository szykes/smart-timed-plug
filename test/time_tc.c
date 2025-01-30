#include "time.h"

#include "framework.h"

static bool tc_test(void) {
  TEST_BEGIN();


  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_test());
  TEST_EVALUATE_END();
}
