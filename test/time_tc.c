#include "time.h"

#include "avr.h"
#include "button.h"

#include "framework.h"

#include <limits.h>
#include <stdio.h>

#define START_ADDR_BASE_TIME (0u)

#define MAX_TIME (999u)
#define MIN_TIME (1u)
#define DEFAULT_TIME (550u)

#define CNT_LIMIT_SHORT ((100u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_LONG ((500u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_VERY_LONG ((1000u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

#define CNT_SET_SLOW_PERIOD (2000u / CNT_LIMIT_LONG)
#define CNT_SET_STANDBY_PERIOD 10u // (30000u / CNT_LIMIT_VERY_LONG)

typedef enum {
  RELAY_NA = 0,
  RELAY_SET,
  RELAY_RESET,
} relay_state_e;

typedef enum {
  MAIN_NA = 0,
  MAIN_START_CNT_RESET = 1,
  MAIN_TICKING = 2,
} main_state_e;

static void button_mock_init(button_event_e buttons) {
  void *param_ptr;
  mock_prepare_param(param_ptr, buttons);

  type_st ret = {
    .type = TYPE_BUTTON_E,
    .value = param_ptr,
    .size = sizeof(buttons),
  };
  mock_initiate_expectation("button_is_pushed", NULL, 0, &ret);
}

static void call_main(button_event_e buttons, relay_state_e relay, main_state_e main) {
  button_mock_init(buttons);
  if (main == MAIN_START_CNT_RESET) {
    mock_initiate_expectation("mcu_cli", NULL, 0, NULL);
    mock_initiate_expectation("mcu_sei", NULL, 0, NULL);
  }
  switch (relay) {
  case RELAY_SET:
    mock_initiate_expectation("gpio_relay_set", NULL, 0, NULL);
    break;
  case RELAY_RESET:
    mock_initiate_expectation("gpio_relay_reset", NULL, 0, NULL);
    break;
  case RELAY_NA:
  default:
    break;
  }
  mock_initiate_expectation("mcu_cli", NULL, 0, NULL);
  mock_initiate_expectation("mcu_sei", NULL, 0, NULL);
  if (main == MAIN_START_CNT_RESET ||
    main == MAIN_TICKING) {
    mock_initiate_expectation("mcu_cli", NULL, 0, NULL);
    mock_initiate_expectation("mcu_sei", NULL, 0, NULL);
  }
  time_main();
}

static void mock_eeprom_load(size_t addr, uint8_t data) {
  void *addr_ptr;
  mock_prepare_param(addr_ptr, addr);

  void *data_ptr;
  mock_prepare_param(data_ptr, data);

  type_st params[] = {
    {
      .type = TYPE_SIZE_T,
      .value = addr_ptr,
      .size = sizeof(addr),
    },
  };

  type_st ret = {
    .type = TYPE_UINT8_T,
    .value = data_ptr,
    .size = sizeof(data),
  };

  mock_initiate_expectation("eeprom_load", params, sizeof(params)/sizeof(type_st), &ret);
}

static void mock_eeprom_store(size_t addr, uint8_t data) {
  void *addr_ptr;
  mock_prepare_param(addr_ptr, addr);

  void *data_ptr;
  mock_prepare_param(data_ptr, data);

  type_st params[] = {
    {
      .type = TYPE_SIZE_T,
      .value = addr_ptr,
      .size = sizeof(addr),
    },
    {
      .type = TYPE_UINT8_T,
      .value = data_ptr,
      .size = sizeof(data),
    },
  };
  mock_initiate_expectation("eeprom_store", params, sizeof(params)/sizeof(type_st), NULL);
}

static bool tc_init(void) {
  TEST_BEGIN();

  mock_eeprom_load(START_ADDR_BASE_TIME, (DEFAULT_TIME >> CHAR_BIT) & 0xFF);
  mock_eeprom_load(START_ADDR_BASE_TIME + 1, DEFAULT_TIME & 0xFF);

  time_init();

  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "tc_init()");

  TEST_END();
}

static bool tc_goes_standby(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < (CNT_LIMIT_VERY_LONG * CNT_SET_STANDBY_PERIOD); i++) {
    char str[30];
    sprintf(str, "i: %lu", (unsigned long)i);
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    time_interrupt();
    TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, str);
  }

  for (size_t i = TIME_STANDBY_START; i < TIME_STANDBY_END; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_VERY_LONG - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    if (i == (TIME_STANDBY_END - 1)) {
      TEST_ASSERT_EQ(time_get_for_display(), i, "");
    } else {
      TEST_ASSERT_EQ(time_get_for_display(), i + 1, "");
    }
  }

  TEST_END();
}

static bool tc_simple_countdown(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME - 1, "countdown just started");

  for (size_t i = DEFAULT_TIME - 1; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "internal counter is incrementing without countdown");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i - 1, "countdowning with 1");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), 0, "countdown just ended");

  for (size_t i = 0; i < (CNT_LIMIT_VERY_LONG - 1); i++) {
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), 0, "showing 00.0");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "set to RESET");

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "back to init");

  TEST_END();
}

static bool tc_countdown_with_pause(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME - 1, "");

  for (size_t i = DEFAULT_TIME / 2; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) + i - 1, "internal counter is incrementing without countdown - before pause");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) + i - 2, "countdowning with 1 - before pause");
  }

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_RESET, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) - 1, "countdown paused");

  for (size_t i = 0; i < 200; i++) {
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) - 1, "countdown paused");
  }

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET);
  TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) - 2, "continue countdown");

  for (size_t i = (DEFAULT_TIME / 2) - 1; i > 1; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i - 1, "internal counter is incrementing without countdown - after pause");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i - 2, "countdowning with 1 - after pause");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), 0, "countdown just ended");

  for (size_t i = 0; i < (CNT_LIMIT_VERY_LONG - 1); i++) {
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), 0, "showing 00.0");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "set to RESET");

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "back to init");

  TEST_END();
}

static bool tc_countdown_with_reset_in_pause(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME - 1, "");

  for (size_t i = DEFAULT_TIME / 2; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) + i - 1, "internal counter is incrementing without countdown - before pause");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) + i - 2, "countdowning with 1 - before pause");
  }

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_RESET, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) - 1, "countdown paused");

  for (size_t i = 0; i < 200; i++) {
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), (DEFAULT_TIME / 2) - 1, "countdown paused");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_START_STOP_LONG, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "");

  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "");

  TEST_END();
}

static bool tc_blocked_time_change_during_countdown(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME - 1, "countdown just started");

  for (size_t i = DEFAULT_TIME - 1; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "internal counter is incrementing without countdown");

      call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "internal counter is incrementing without countdown");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i - 1, "countdowning with 1");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), 0, "countdown just ended");

  for (size_t i = 0; i < (CNT_LIMIT_VERY_LONG - 1); i++) {
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), 0, "showing 00.0");

    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), 0, "showing 00.0");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "set to RESET");

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "back to init");

  TEST_END();
}

static bool tc_change_time_in_state_reset(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME + 1, "increase base time");

  for (size_t i = DEFAULT_TIME + 1; i < (DEFAULT_TIME + 3); i++) {
    for (size_t j = 0; j < (CNT_LIMIT_LONG - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "increase base time");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i + 1, "increase base time");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME + 3, "do nothing");

  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME + 2, "decrease base time");

  for (size_t i = DEFAULT_TIME + 2; i > DEFAULT_TIME; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_LONG - 1); j++) {
      time_interrupt();
      call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
      TEST_ASSERT_EQ(time_get_for_display(), i, "decrease base time");
    }

    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i - 1, "decrease base time");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "do nothing");

  TEST_END();
}

bool tc_set_max_and_min_base_time_default_to_max(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME + 1, "increase base time");

  size_t slow_period_cnt = 0;
  for (size_t i = DEFAULT_TIME + 1; i < MAX_TIME; i++) {
    if (slow_period_cnt < CNT_SET_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_LONG - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "increase base time");
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "increase base time");
      }
    }

    slow_period_cnt++;

    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i + 1, "increase base time");
  }

  for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), MAX_TIME, "max base time");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), MAX_TIME, "increase base time");

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), MAX_TIME, "do nothing");

  TEST_END();
}

bool tc_set_max_and_min_base_time_max_to_min(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), (MAX_TIME - 1), "decrease base time");

  size_t slow_period_cnt = 0;
  for (size_t i = MAX_TIME - 1; i > MIN_TIME; i--) {
    if (slow_period_cnt < CNT_SET_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_LONG - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "decrease base time");
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "decrease base time");
      }
    }

    slow_period_cnt++;

    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i - 1, "decrease base time");
  }

  for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA);
    TEST_ASSERT_EQ(time_get_for_display(), MIN_TIME, "max base time");
  }

  time_interrupt();
  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), MIN_TIME, "increase base time");

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), MIN_TIME, "do nothing");

  TEST_END();
}

bool tc_set_max_and_min_base_time_min_to_default(void) {
  TEST_BEGIN();

  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
  TEST_ASSERT_EQ(time_get_for_display(), MIN_TIME + 1, "increase base time");

  size_t slow_period_cnt = 0;
  for (size_t i = MIN_TIME + 1; i < DEFAULT_TIME; i++) {
    if (slow_period_cnt < CNT_SET_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_LONG - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "increase base time");
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_SHORT - 1); j++) {
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA);
	TEST_ASSERT_EQ(time_get_for_display(), i, "increase base time");
      }
    }

    slow_period_cnt++;

    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING);
    TEST_ASSERT_EQ(time_get_for_display(), i + 1, "increase base time");
  }

  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA);
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "do nothing");

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_init());
  TEST_EVALUATE(tc_goes_standby());
  TEST_EVALUATE(tc_simple_countdown());
  TEST_EVALUATE(tc_countdown_with_pause());
  TEST_EVALUATE(tc_countdown_with_reset_in_pause());
  TEST_EVALUATE(tc_blocked_time_change_during_countdown());
  TEST_EVALUATE(tc_change_time_in_state_reset());
  TEST_EVALUATE(tc_set_max_and_min_base_time_default_to_max());
  TEST_EVALUATE(tc_set_max_and_min_base_time_max_to_min());
  TEST_EVALUATE(tc_set_max_and_min_base_time_min_to_default());
  TEST_EVALUATE_END();
}
