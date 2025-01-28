#include "time.h"

#include "avr.h"
#include "button.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_TIME (999u)

#define CNT_LIMIT_100_MS ((100u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_500_MS ((500u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

#define CNT_SET_SLOW_PERIOD_2_S ((2000u / CNT_LIMIT_500_MS) - 1)

typedef enum {
  TIME_RESET = 0,
  TIME_START = 1,
  TIME_DECREASING = 2,
  TIME_PAUSE = 3,
} time_state_e;

// The time_set contains the time period for how long the relay turns on
// in tenth of a second. So, 150 means 15,0 seconds.
static uint16_t time_set = 150;
static uint16_t time_cnt;
static time_state_e time_cnt_state;

// This is a counter for a tick.
volatile static uint8_t cnt_for_tick;
static uint8_t cnt_limit_for_tick;

static uint8_t cnt_for_slow;

static bool is_first_button_plus_minus_pushed;

static void decrement_time_cnt(void) {
  if (time_cnt > 0) {
    time_cnt--;
  }
}

static void increment_time_set(void) {
  if (time_set < MAX_TIME) {
    time_set++;
  }
}

static void decrement_time_set(void) {
  if (time_set > 0) {
    time_set--;
  }
}

static void do_action_at_tick(button_e pushed_button) {
  if (time_cnt_state == TIME_DECREASING) {
    decrement_time_cnt();
  } else if (pushed_button == BUTTON_PLUS) {
    increment_time_set();
    cnt_for_slow++;
  } else if (pushed_button == BUTTON_MINUS) {
    decrement_time_set();
    cnt_for_slow++;
  }
}

static bool evaluate_cnt_for_tick(void) {
  bool is_ticking = false;

  mcu_cli();
  if (cnt_limit_for_tick > 0) {
    if (cnt_for_tick >= cnt_limit_for_tick) {
      is_ticking = true;
    }
  } else {
    cnt_for_tick = 0;
  }
  mcu_sei();

  return is_ticking;
}

static void handle_button_start_stop(void) {
  switch (time_cnt_state) {
  case TIME_RESET:
  case TIME_PAUSE:
    time_cnt_state = TIME_START;
    cnt_limit_for_tick = CNT_LIMIT_100_MS;
    gpio_relay_set();
    decrement_time_cnt();
    break;

  case TIME_DECREASING:
    time_cnt_state = TIME_PAUSE;
    cnt_limit_for_tick = 0;
    gpio_relay_reset();
    break;

  default:
    break;
  }
}

static void handle_button_plus_minus(button_e pushed_button) {
  if (time_cnt_state != TIME_RESET) {
    return;
  }

  if (cnt_for_slow < CNT_SET_SLOW_PERIOD_2_S) {
    cnt_limit_for_tick = CNT_LIMIT_500_MS;
  } else {
    cnt_limit_for_tick = CNT_LIMIT_100_MS;
  }

  if (!is_first_button_plus_minus_pushed) {
    if (pushed_button == BUTTON_PLUS) {
      increment_time_set();
    } else {
      decrement_time_set();
    }
    is_first_button_plus_minus_pushed = true;
  }
}

static void handle_button_released(void) {
  switch (time_cnt_state) {
  case TIME_RESET:
    time_cnt = time_set;
    cnt_limit_for_tick = 0;
    break;

  case TIME_START:
    time_cnt_state = TIME_DECREASING;
    break;

  default:
    break;
  }

  is_first_button_plus_minus_pushed = false;
}

void time_5ms_task(void) {
  cnt_for_tick++;
}

void time_main(void) {
  button_e pushed_button = button_is_pushed();
  switch (pushed_button) {
  case BUTTON_START_STOP:
    handle_button_start_stop();
    break;

  case BUTTON_PLUS:
  case BUTTON_MINUS:
    handle_button_plus_minus(pushed_button);
    break;

  case BUTTON_RELEASED:
    handle_button_released();
    break;
  }

  if (time_cnt == 0) {
    time_cnt_state = TIME_RESET;
    cnt_limit_for_tick = 0;
    gpio_relay_reset();
  }

  if (evaluate_cnt_for_tick()) {
    do_action_at_tick(pushed_button);
  }
}
