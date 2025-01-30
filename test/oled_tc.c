#include "oled.h"

#include "framework.h"

static bool tc_test(void) {
  TEST_BEGIN();

  oled_update_screen();

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_test());
  TEST_EVALUATE_END();
}
