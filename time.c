#include "time.h"

#include "avr.h"
#include "button.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_TIME (999u)
#define MIN_TIME (1u)
#define DEFAULT_TIME (150u)

#define CNT_LIMIT_SHORT ((100u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_LONG ((500u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_VERY_LONG ((1000u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

#define CNT_SET_SLOW_PERIOD ((2000u / CNT_LIMIT_LONG) - 1)

typedef enum {
  TIME_RESET = 0,
  TIME_START = 1,
  TIME_DECREASING = 2,
  TIME_PAUSING = 3,
  TIME_PAUSED = 4,
  TIME_END = 5,
} time_state_e;

// The time_set contains the time period for how long the relay turns on
// in tenth of a second. So, 150 means 15,0 seconds.
static uint16_t base_time = DEFAULT_TIME;
static uint16_t time_cnt;
static time_state_e time_cnt_state;

// This is a counter for a tick.
volatile static uint8_t cnt_for_tick;
static uint8_t cnt_limit_for_tick;

static uint8_t cnt_for_slow;

static bool is_first_button_plus_minus_pushed;

static void reset_cnt_for_tick(void) {
  mcu_cli();
  cnt_for_tick = 0;
  mcu_sei();
}

static void decrement_time_cnt(void) {
  if (time_cnt > 0) {
    time_cnt--;
  }
  reset_cnt_for_tick();
}

static void increment_time_set(void) {
  if (base_time < MAX_TIME) {
    base_time++;
  }
  reset_cnt_for_tick();
}

static void decrement_time_set(void) {
  if (base_time > MIN_TIME) {
    base_time--;
  }
  reset_cnt_for_tick();
}

static void increment_cnt_for_slow(void) {
  if (cnt_for_slow < CNT_SET_SLOW_PERIOD) {
    cnt_for_slow++;
  }
}

static void do_action_at_tick(button_event_e pushed_button) {
  if (time_cnt_state == TIME_DECREASING) {
    decrement_time_cnt();
  } else if (time_cnt_state == TIME_END) {
    time_cnt_state = TIME_RESET;
  } else if (pushed_button == BUTTON_EVENT_PLUS) {
    increment_time_set();
    increment_cnt_for_slow();
  } else if (pushed_button == BUTTON_EVENT_MINUS) {
    decrement_time_set();
    increment_cnt_for_slow();
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

static void handle_button_start_stop(button_event_e pushed_button) {
  switch (time_cnt_state) {
  case TIME_PAUSED:
    if (pushed_button == BUTTON_EVENT_START_STOP_LONG) {
      time_cnt_state = TIME_RESET;
      break;
    }
    // wanted fall through
  case TIME_RESET:
    time_cnt_state = TIME_START;
    cnt_limit_for_tick = CNT_LIMIT_SHORT;
    reset_cnt_for_tick();
    gpio_relay_set();
    decrement_time_cnt();
    break;

  case TIME_DECREASING:
    time_cnt_state = TIME_PAUSING;
    cnt_limit_for_tick = 0;
    gpio_relay_reset();
    break;

  default:
    break;
  }
}

static void handle_button_plus_minus(button_event_e pushed_button) {
  if (!(time_cnt_state == TIME_RESET || time_cnt_state == TIME_PAUSED)) {
    return;
  }

  if (cnt_for_slow < CNT_SET_SLOW_PERIOD) {
    cnt_limit_for_tick = CNT_LIMIT_LONG;
  } else {
    cnt_limit_for_tick = CNT_LIMIT_SHORT;
  }

  if (!is_first_button_plus_minus_pushed) {
    if (pushed_button == BUTTON_EVENT_PLUS) {
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
    time_cnt = base_time;
    cnt_limit_for_tick = 0;
    break;

  case TIME_PAUSING:
    time_cnt_state = TIME_PAUSED;
    break;

  case TIME_START:
    time_cnt_state = TIME_DECREASING;
    break;

  default:
    break;
  }

  cnt_for_slow = 0;
  is_first_button_plus_minus_pushed = false;
}

void time_interrupt(void) {
  cnt_for_tick++;
}

void time_main(void) {
  button_event_e pushed_button = button_is_pushed();
  switch (pushed_button) {
  case BUTTON_EVENT_START_STOP_SHORT:
  case BUTTON_EVENT_START_STOP_LONG:
    handle_button_start_stop(pushed_button);
    break;

  case BUTTON_EVENT_PLUS:
  case BUTTON_EVENT_MINUS:
    handle_button_plus_minus(pushed_button);
    break;

  case BUTTON_EVENT_RELEASED:
    handle_button_released();
    break;
  }

  if (time_cnt == 0 && time_cnt_state != TIME_END) {
    time_cnt_state = TIME_END;
    cnt_limit_for_tick = CNT_LIMIT_VERY_LONG;
    gpio_relay_reset();
  }

  if (evaluate_cnt_for_tick()) {
    do_action_at_tick(pushed_button);
  }
}

uint16_t time_get_for_display(void) {
  if (time_cnt_state != TIME_RESET) {
    return time_cnt;
  }

  return base_time;
}
