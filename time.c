#include "time.h"

#include "avr.h"
#include "button.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define START_ADDR_BASE_TIME (0u)

#define MAX_TIME (999u)
#define MIN_TIME (1u)

#define CNT_LIMIT_SHORT ((100u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_LONG ((500u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define CNT_LIMIT_VERY_LONG ((1000u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

#define CNT_SET_SLOW_PERIOD (2000u / CNT_LIMIT_LONG)
#define CNT_SET_STANDBY_PERIOD (30000u / CNT_LIMIT_VERY_LONG)

typedef enum {
  TIME_RESET = 0,
  TIME_STANDBY = 1,
  TIME_START = 2,
  TIME_DECREASING = 3,
  TIME_PAUSING = 4,
  TIME_PAUSED = 5,
  TIME_END = 6,
} time_state_e;

// The time_set contains the time period for how long the relay turns on
// in tenth of a second. So, 150 means 15,0 seconds.
static uint16_t base_time;
static bool is_changed_base_time;
static uint16_t time_cnt;
static time_state_e time_cnt_state;

// This is a counter for a tick.
volatile static uint8_t cnt_for_tick;
static uint8_t cnt_limit_for_tick;

static uint8_t cnt_for_slow;
static uint8_t cnt_for_standby;
static uint16_t standby_cnt = TIME_STANDBY_START;

static bool is_first_button_plus_minus_pushed;

static void load_base_time(void) {
  base_time = eeprom_load(START_ADDR_BASE_TIME) << CHAR_BIT;
  base_time |= eeprom_load(START_ADDR_BASE_TIME + 1);
}

static void store_base_time(void) {
  if (is_changed_base_time) {
    eeprom_store(START_ADDR_BASE_TIME, base_time >> CHAR_BIT);
    eeprom_store(START_ADDR_BASE_TIME + 1, base_time);
    is_changed_base_time = false;
  }
}

static void reset_cnt_for_tick(void) {
  mcu_cli();
  cnt_for_tick = 0;
  mcu_sei();
}

static void decrement_time_cnt(void) {
  if (time_cnt > 0) {
    time_cnt--;
  }
}

static void increment_time_set(void) {
  if (base_time < MAX_TIME) {
    base_time++;
    is_changed_base_time = true;
  }
}

static void decrement_time_set(void) {
  if (base_time > MIN_TIME) {
    base_time--;
    is_changed_base_time = true;
  }
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
  } else {
    if (time_cnt_state == TIME_STANDBY) {
      if (standby_cnt < TIME_STANDBY_END) {
	standby_cnt++;
      } else {
	standby_cnt = TIME_STANDBY_START;
      }
    }

    if (time_cnt_state == TIME_PAUSED ||
	time_cnt_state == TIME_RESET) {
      if (cnt_for_standby < CNT_SET_STANDBY_PERIOD) {
	cnt_for_standby++;
      } else {
	store_base_time();
	time_cnt_state = TIME_STANDBY;
	standby_cnt = TIME_STANDBY_START;
	cnt_for_standby = 0;
      }
    }
  }
  reset_cnt_for_tick();
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
      reset_cnt_for_tick();
      break;
    }
    // wanted fall through
  case TIME_STANDBY:
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

  cnt_for_standby = 0;
}

static void handle_button_plus_minus(button_event_e pushed_button) {
  if (!(time_cnt_state == TIME_RESET ||
	time_cnt_state == TIME_PAUSED ||
	time_cnt_state == TIME_STANDBY)) {
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
    reset_cnt_for_tick();
    is_first_button_plus_minus_pushed = true;
  }

  cnt_for_standby = 0;
}

static void handle_button_released(void) {
  switch (time_cnt_state) {
  case TIME_RESET:
    time_cnt = base_time;
  case TIME_PAUSED:
    cnt_limit_for_tick = CNT_LIMIT_VERY_LONG;
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

void time_init(void) {
  load_base_time();
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
  if (time_cnt_state == TIME_STANDBY) {
    return standby_cnt;
  }

  if (time_cnt_state != TIME_RESET) {
    return time_cnt;
  }

  return base_time;
}
