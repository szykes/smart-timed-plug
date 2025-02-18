#include "time.h"

#include "avr.h"
#include "button.h"
#include "oled_conf.h"

#include "framework.h"

#include <limits.h>
#include <stdio.h>

#define START_ADDR_BASE_TIME (0u)

#define MAX_TIME (999u)
#define MIN_TIME (1u)
#define DEFAULT_TIME (150u)

#define SEC (1000u)

#define LIMIT_COUNTDOWN (100u)
#define LIMIT_CHANGING_TIME_SLOW (400u)
#define LIMIT_CHANGING_TIME_FAST (100u)
#define LIMIT_PLAYING_STNDBY_PICTURES (400u)
#define LIMIT_WAITING_AT_END (1u * SEC)
#define LIMIT_WAITING_STANDBY (1u * SEC)

#define CNT_LIMIT_COUNTDOWN ((LIMIT_COUNTDOWN / TIMER_INTERRUPT_PERIOD_TIME) - 1u)
#define CNT_LIMIT_CHANGING_TIME_SLOW ((LIMIT_CHANGING_TIME_SLOW / TIMER_INTERRUPT_PERIOD_TIME) - 1u)
#define CNT_LIMIT_CHANGING_TIME_FAST ((LIMIT_CHANGING_TIME_FAST / TIMER_INTERRUPT_PERIOD_TIME) - 1u)
#define CNT_LIMIT_PLAYING_STNDBY_PICTURES ((LIMIT_PLAYING_STNDBY_PICTURES / TIMER_INTERRUPT_PERIOD_TIME) - 1u)
#define CNT_LIMIT_WAITING_AT_END ((LIMIT_WAITING_AT_END / TIMER_INTERRUPT_PERIOD_TIME) - 1u)
#define CNT_LIMIT_WAITING_STANDBY ((LIMIT_WAITING_STANDBY / TIMER_INTERRUPT_PERIOD_TIME) - 1u)

#define CNT_FOR_RESET ((2 * SEC) / LIMIT_WAITING_AT_END)
#define CNT_FOR_CHANGING_TIME_SLOW_PERIOD ((2 * SEC) / LIMIT_CHANGING_TIME_SLOW)
#define CNT_FOR_STANDBY ((120 * SEC) / LIMIT_WAITING_STANDBY)

#define DEFAULT_PROGRESS_PIXELS (0u)

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

static uint8_t calc_progress(uint16_t base_time, uint16_t time_cnt, uint8_t disp_pixels) {
  return (disp_pixels * (base_time - time_cnt)) / base_time;
}

static void call_main(button_event_e buttons, relay_state_e relay, main_state_e main, int16_t eeprom, const char *msg) {
  MOCK_EXPECT_RET("button_is_pushed", button_event_e, buttons, msg);
  if (main == MAIN_START_CNT_RESET) {
    MOCK_EXPECT("mcu_cli", msg);
    MOCK_EXPECT("mcu_sei", msg);
  }
  switch (relay) {
  case RELAY_SET:
    MOCK_EXPECT("gpio_relay_set", msg);
    break;
  case RELAY_RESET:
    MOCK_EXPECT("gpio_relay_reset", msg);
    break;
  case RELAY_NA:
  default:
    break;
  }

  MOCK_EXPECT("mcu_cli", msg);
  MOCK_EXPECT("mcu_sei", msg);

  if (eeprom >= 0) {
    mock_skip addr = 0;
    uint16_t data = eeprom;
    MOCK_EXPECT_2_PARAM("eeprom_store", mock_skip, addr, uint16_t, data, msg);
  }

  if (main == MAIN_TICKING) {
    MOCK_EXPECT("mcu_cli", msg);
    MOCK_EXPECT("mcu_sei", msg);
  }
  time_main();
}

static bool tc_init(void) {
  TEST_BEGIN();

  mock_skip addr = 0;
  uint16_t data = DEFAULT_TIME;
  MOCK_EXPECT_1_PARAM_RET("eeprom_load", mock_skip, addr, uint16_t, data, "");

  time_init();

  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, "tc_init()");
  TEST_ASSERT_EQ(time_get_for_display(), DEFAULT_TIME, "tc_init()");

  TEST_END();
}

static bool tc_goes_standby(void) {
  TEST_BEGIN();
  char msg[50];

  for (size_t i = 0; i < CNT_FOR_STANDBY; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_WAITING_STANDBY - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "before standby, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);
    }

    if (i == (CNT_FOR_STANDBY - 1)) {
      break;
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "before standby, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "just before standby");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), TIME_STANDBY_START, msg);

  for (size_t i = TIME_STANDBY_START; i <= TIME_STANDBY_END; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_PLAYING_STNDBY_PICTURES - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "standby 1st round, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "standby 1st round, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    if (i == (TIME_STANDBY_END)) {
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), TIME_STANDBY_START, msg);
    } else {
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i + 1, msg);
    }
  }

  for (size_t i = TIME_STANDBY_START; i <= TIME_STANDBY_END; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_PLAYING_STNDBY_PICTURES - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "standby 2nd round, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "standby 2nd round, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    if (i == (TIME_STANDBY_END)) {
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), TIME_STANDBY_START, msg);
    } else {
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i + 1, msg);
    }
  }

  TEST_END();
}

static bool tc_simple_countdown(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "start button pushed");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME - 1, msg);

  for (size_t i = DEFAULT_TIME - 1; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i - 1, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "countdown just ended");
  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);

  for (size_t i = 0; i < CNT_FOR_RESET; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_WAITING_AT_END - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "shown 00.0, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
    }

    if (i == (CNT_FOR_RESET - 1)) {
      break;
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "shown 00.0, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "cnt state to RESET");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "back to init");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

static bool tc_countdown_with_pause(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "start button pushed");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME - 1, msg);

  for (size_t i = DEFAULT_TIME / 2; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown before pause, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 1, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 1, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown before pause, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 2, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 2, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "countdown is just paused");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);

  for (size_t i = 0; i < (CNT_LIMIT_WAITING_STANDBY - 1); i++) {
    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown paused, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "continue countdown");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 2, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 2, msg);

  for (size_t i = (DEFAULT_TIME / 2) - 1; i > 1; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown after pause, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i - 1, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 1, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown after pause, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i - 2, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 2, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "countdown just ended");
  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);

  for (size_t i = 0; i < CNT_FOR_RESET; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_WAITING_AT_END - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "showing 00.0, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
    }

    if (i == (CNT_FOR_RESET - 1)) {
      break;
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "showing 00.0, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "set to RESET");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "back to init");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

static bool tc_countdown_with_reset_in_pause(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "push start");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME - 1, msg);

  for (size_t i = DEFAULT_TIME / 2; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown before pause, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 1, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 1, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown before pause, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 2, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 2, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "countdown paused");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);

  for (size_t i = 0; i < (CNT_LIMIT_WAITING_STANDBY - 2); i++) {
    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown paused, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "long pushed start");
  time_interrupt();
  call_main(BUTTON_EVENT_START_STOP_LONG, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "back to init");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

static bool tc_blocked_time_change_during_countdown(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "start button pushed");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME - 1, msg);

  for (size_t i = DEFAULT_TIME - 1; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown - try to increment time, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);

      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown - try to decrement time, i: %lu, j: %lu", i, j);
      call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, i - 1, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "countdown just ended");
  call_main(BUTTON_EVENT_RELEASED, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);

  for (size_t i = 0; i < CNT_FOR_RESET; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_WAITING_AT_END - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "1st showing 00.0, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);

      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "2nd showing 00.0, i: %lu, j: %lu", i, j);
      call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
    }

    if (i == (CNT_FOR_RESET - 1)) {
      break;
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "showing 00.0, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, 0, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), 0, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "set to RESET");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "back to init");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

static bool tc_change_time_in_state_reset(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "increase base time");
  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME + 1, msg);

  for (size_t i = DEFAULT_TIME + 1; i < (DEFAULT_TIME + 3); i++) {
    for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_SLOW - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "increase base time, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "increase base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i + 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "do nothing");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME + 3, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "decrease base time");
  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME + 2, msg);

  for (size_t i = DEFAULT_TIME + 2; i > DEFAULT_TIME; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_SLOW - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "decrease base time, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "decrease base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "do nothing");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

bool tc_set_max_and_min_base_time_default_to_max(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "increase base time");
  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME + 1, msg);

  size_t slow_period_cnt = 0;
  for (size_t i = DEFAULT_TIME + 1; i < MAX_TIME; i++) {
    if (slow_period_cnt < CNT_FOR_CHANGING_TIME_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_SLOW - 1); j++) {
	msg[0] = '\0';
	snprintf(msg, sizeof(msg), "increase base time, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_FAST - 1); j++) {
	msg[0] = '\0';
        snprintf(msg, sizeof(msg), "increase base time - at slow, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    }

    slow_period_cnt++;

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "increase base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i + 1, msg);
  }

  for (size_t i = 0; i < (CNT_LIMIT_CHANGING_TIME_FAST - 1); i++) {
    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "max base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MAX_TIME, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "increase base time");
  time_interrupt();
  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MAX_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "do nothing");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MAX_TIME, msg);

  TEST_END();
}

bool tc_set_max_and_min_base_time_max_to_min(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "decrease base time");
  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (MAX_TIME - 1), msg);

  size_t slow_period_cnt = 0;
  for (size_t i = MAX_TIME - 1; i > MIN_TIME; i--) {
    if (slow_period_cnt < CNT_FOR_CHANGING_TIME_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_SLOW - 1); j++) {
	msg[0] = '\0';
	snprintf(msg, sizeof(msg), "decrease base time, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_FAST - 1); j++) {
	msg[0] = '\0';
        snprintf(msg, sizeof(msg), "decrease base time - at slow, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    }

    slow_period_cnt++;

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "decrease base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i - 1, msg);
  }

  for (size_t i = 0; i < (CNT_LIMIT_CHANGING_TIME_FAST - 1); i++) {
    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "min base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_NA, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MIN_TIME, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "decrease base time");
  time_interrupt();
  call_main(BUTTON_EVENT_MINUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MIN_TIME, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "do nothing");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MIN_TIME, msg);

  TEST_END();
}

bool tc_set_max_and_min_base_time_min_to_default(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "increase base time");
  call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), MIN_TIME + 1, msg);

  size_t slow_period_cnt = 0;
  for (size_t i = MIN_TIME + 1; i < DEFAULT_TIME; i++) {
    if (slow_period_cnt < CNT_FOR_CHANGING_TIME_SLOW_PERIOD) {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_SLOW - 1); j++) {
	msg[0] = '\0';
	snprintf(msg, sizeof(msg), "increase base time, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    } else {
      for (size_t j = 0; j < (CNT_LIMIT_CHANGING_TIME_FAST - 1); j++) {
	msg[0] = '\0';
        snprintf(msg, sizeof(msg), "increase base time - at slow, i: %lu, j: %lu", i, j);
	time_interrupt();
	call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_NA, -1, msg);
	TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
	TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i, msg);
      }
    }

    slow_period_cnt++;

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "increase base time, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_PLUS, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), i + 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "do nothing");
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), DEFAULT_PROGRESS_PIXELS, msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME, msg);

  TEST_END();
}

static bool tc_countdown_with_pause_goes_standby(void) {
  TEST_BEGIN();
  char msg[50];

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "start button pushed");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_SET, MAIN_START_CNT_RESET, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), DEFAULT_TIME - 1, msg);

  for (size_t i = DEFAULT_TIME / 2; i > 0; i--) {
    for (size_t j = 0; j < (CNT_LIMIT_COUNTDOWN - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "countdown before pause, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 1, OLED_COLS), msg)
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 1, msg);
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "countdown before pause, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) + i - 2, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) + i - 2, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "1st countdown is just paused");
  call_main(BUTTON_EVENT_START_STOP_SHORT, RELAY_RESET, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "2nd countdown is just paused");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);

  for (size_t i = 0; i < CNT_FOR_STANDBY; i++) {
    for (size_t j = 0; j < (CNT_LIMIT_WAITING_STANDBY - 1); j++) {
      msg[0] = '\0';
      snprintf(msg, sizeof(msg), "before standby, i: %lu, j: %lu", i, j);
      time_interrupt();
      call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_NA, -1, msg);
      TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
      TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);
    }

    if (i == (CNT_FOR_STANDBY - 1)) {
      break;
    }

    msg[0] = '\0';
    snprintf(msg, sizeof(msg), "before standby, i: %lu", i);
    time_interrupt();
    call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, -1, msg);
    TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, (DEFAULT_TIME / 2) - 1, OLED_COLS), msg);
    TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), (DEFAULT_TIME / 2) - 1, msg);
  }

  msg[0] = '\0';
  snprintf(msg, sizeof(msg), "just before standby");
  time_interrupt();
  call_main(BUTTON_EVENT_RELEASED, RELAY_NA, MAIN_TICKING, DEFAULT_TIME, msg);
  TEST_ASSERT_EQ(time_get_progress_in_pixels(OLED_COLS), calc_progress(DEFAULT_TIME, DEFAULT_TIME, OLED_COLS), msg);
  TEST_ASSERT_EQ_WITH_MOCK(time_get_for_display(), TIME_STANDBY_START, msg);

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
  TEST_EVALUATE(tc_countdown_with_pause_goes_standby());
  TEST_EVALUATE_END();
}
