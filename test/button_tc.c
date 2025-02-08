#include "button.h"

#include "avr.h"

#include "framework.h"

#include <stdint.h>

#define TEST_DEBOUNCING_CNT ((10u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define LONG_CNT ((1000u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

struct button_tc_st {
  const char *name;
  uint8_t input_button;
  button_event_e pushed_button;
};

struct button_tc_st plus_minus_button_tcs[] = {
  {
    .name = "plus button",
    .input_button = GPIO_BTN_PLUS,
    .pushed_button = BUTTON_EVENT_PLUS,
  },
  {
    .name = "minus button",
    .input_button = GPIO_BTN_MINUS,
    .pushed_button = BUTTON_EVENT_MINUS,
  },
};

struct multiple_buttons_tc {
  const char *name;
  uint8_t buttons;
};

struct multiple_buttons_tc multiple_buttons_tcs[] = {
  {
    .name = "start/stop + plus buttons",
    .buttons = GPIO_BTN_START_STOP | GPIO_BTN_PLUS,
  },
  {
    .name = "start/stop + minus buttons",
    .buttons = GPIO_BTN_START_STOP | GPIO_BTN_MINUS,
  },
  {
    .name = "plus + minus buttons",
    .buttons = GPIO_BTN_PLUS | GPIO_BTN_MINUS,
  },
  {
    .name = "start/stop + plus + minus buttons",
    .buttons = GPIO_BTN_START_STOP | GPIO_BTN_PLUS | GPIO_BTN_MINUS,
  },
};

static void call_button_main(int16_t buttons) {
  MOCK_EXPECT("mcu_cli", "");
  MOCK_EXPECT("mcu_sei", "");
  if (buttons >= 0) {
    uint8_t btns = buttons;
    MOCK_EXPECT_RET("gpio_inputs_get", TYPE_UINT8_T, btns, "");
  }
  button_main();
}

static bool tc_nothing_happens(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < 500; i++) {
    call_button_main(-1);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    button_interrupt();
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_plus_minus_moment_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(plus_minus_button_tcs)/sizeof(struct button_tc_st); i++) {
    struct button_tc_st *t = &plus_minus_button_tcs[i];

    call_button_main(-1);

    // 1st iteration
    button_interrupt();
    call_button_main(t->input_button);
    TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

    // 2nd iteration
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

    // 3rd itartion
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, t->name);

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_start_stop_moment_push(void) {
  TEST_BEGIN();

  call_button_main(-1);

  // 1st iteration
  button_interrupt();
  call_button_main(GPIO_BTN_START_STOP);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  // 2nd iteration
  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  // 3rd itartion
  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_START_STOP_SHORT, "");

  // 4th itartion
  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  TEST_END();
}

static bool tc_plus_minus_short_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(plus_minus_button_tcs)/sizeof(struct button_tc_st); i++) {
    struct button_tc_st *t = &plus_minus_button_tcs[i];

    call_button_main(-1);

    // 1st iteration
    button_interrupt();
    call_button_main(t->input_button);
    TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

    // 2nd iteration
    button_interrupt();
    call_button_main(t->input_button);
    TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

    // 3rd itartion
    button_interrupt();
    call_button_main(t->input_button);
    TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

    // 4th itartion
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, t->name);

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_all_short_push(void) {
  TEST_BEGIN();

  call_button_main(-1);

  // 1st iteration
  button_interrupt();
  call_button_main(GPIO_BTN_START_STOP);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  // 2nd iteration
  button_interrupt();
  call_button_main(GPIO_BTN_START_STOP);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  // 3rd itartion
  button_interrupt();
  call_button_main(GPIO_BTN_START_STOP);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  // 4th itartion
  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_START_STOP_SHORT, "");

  // 5th itartion
  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  TEST_END();
}


static bool tc_plus_minus_long_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(plus_minus_button_tcs)/sizeof(struct button_tc_st); i++) {
    struct button_tc_st *t = &plus_minus_button_tcs[i];

    for (size_t i = 0; i < 40; i++) {
      call_button_main(-1);

      button_interrupt();
      call_button_main(t->input_button);
      TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), t->pushed_button, t->name);
    }

    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, t->name);

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_start_stop_longest_short_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < LONG_CNT; i++) {
    button_interrupt();
    call_button_main(GPIO_BTN_START_STOP);
    TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), BUTTON_EVENT_RELEASED, "");
  }

  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_START_STOP_SHORT, "");

  button_interrupt();
  call_button_main(0);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

  TEST_END();
}

static bool tc_start_stop_shortest_long_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < LONG_CNT; i++) {
    button_interrupt();
    call_button_main(GPIO_BTN_START_STOP);
    TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), BUTTON_EVENT_RELEASED, "");
  }

  button_interrupt();
  call_button_main(GPIO_BTN_START_STOP);
  TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_START_STOP_LONG, "");

  for (size_t i = 0; i < (LONG_CNT + 1); i++) {
    button_interrupt();
    call_button_main(GPIO_BTN_START_STOP);
    TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), BUTTON_EVENT_RELEASED, "");
  }

  for (size_t i = 0; i < (LONG_CNT + 1); i++) {
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), BUTTON_EVENT_RELEASED, "");
  }

  TEST_END();
}

static bool tc_plus_minus_intermittent_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(plus_minus_button_tcs)/sizeof(struct button_tc_st); i++) {
    struct button_tc_st *t = &plus_minus_button_tcs[i];

    for (size_t i = 0; i < 2; i++) {
      call_button_main(-1);

      // 1st iteration
      button_interrupt();
      call_button_main(t->input_button);
      TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

      // 2nd iteration
      button_interrupt();
      call_button_main(0);
      TEST_ASSERT_EQ(button_is_pushed(), t->pushed_button, t->name);

      // 3rd itartion
      button_interrupt();
      call_button_main(0);
      TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, t->name);
    }

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_start_stop_intermittent_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < 2; i++) {
    call_button_main(-1);

    // 1st iteration
    button_interrupt();
    call_button_main(GPIO_BTN_START_STOP);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    // 2nd iteration
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    // 3rd itartion
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_START_STOP_SHORT, "");

    // 4th itartion
    button_interrupt();
    call_button_main(0);
    TEST_ASSERT_EQ(button_is_pushed(), BUTTON_EVENT_RELEASED, "");

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tc_multiple_push(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(multiple_buttons_tcs)/sizeof(struct multiple_buttons_tc); i++) {
    struct multiple_buttons_tc *t = &multiple_buttons_tcs[i];

    for (size_t i = 0; i < 10; i++) {
      call_button_main(-1);

      button_interrupt();
      call_button_main(t->buttons);
      TEST_ASSERT_EQ_WITH_MOCK(button_is_pushed(), BUTTON_EVENT_RELEASED, t->name);
    }
  }

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_nothing_happens());
  TEST_EVALUATE(tc_plus_minus_moment_push());
  TEST_EVALUATE(tc_start_stop_moment_push());
  TEST_EVALUATE(tc_plus_minus_short_push());
  TEST_EVALUATE(tc_all_short_push());
  TEST_EVALUATE(tc_plus_minus_long_push());
  TEST_EVALUATE(tc_start_stop_longest_short_push());
  TEST_EVALUATE(tc_start_stop_shortest_long_push());
  TEST_EVALUATE(tc_plus_minus_intermittent_push());
  TEST_EVALUATE(tc_start_stop_intermittent_push());
  TEST_EVALUATE(tc_multiple_push());
  TEST_EVALUATE_END();
}
