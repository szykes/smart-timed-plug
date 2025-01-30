#include "button.h"

#include "framework.h"

#include <stdint.h>

static void input_mock_init(uint8_t buttons) {
  void *param_ptr;
  mock_prepare_param(param_ptr, buttons);

  type_st ret = {
    .type = TYPE_UINT8_T,
    .value = param_ptr,
    .size = sizeof(buttons),
  };
  mock_initiate_expectation("gpio_inputs_get", NULL, 0, &ret);
}

static void call_button_main(void) {
  mock_initiate_expectation("mcu_cli", NULL, 0, NULL);
  mock_initiate_expectation("mcu_sei", NULL, 0, NULL);
  button_main();
}

static bool tc_nothing_happens(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < 1; i++) {
    call_button_main();
    input_mock_init(0);
    button_5ms_task();
    call_button_main();
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_RELEASED, "");
  }

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_nothing_happens());
  TEST_EVALUATE_END();
}
