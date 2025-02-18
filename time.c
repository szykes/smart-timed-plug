#include "time.h"

#include "avr.h"
#include "button.h"

#ifdef __AVR__
#include <avr/eeprom.h>
#endif // __AVR__

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define START_ADDR_BASE_TIME (0u)

#define MAX_TIME (999u)
#define MIN_TIME (1u)

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
// TODO: make it EEPROM compatible
static uint16_t
#ifdef __AVR__
    EEMEM
#endif // __AVR__
base_time_addr = 150u;
static uint16_t base_time;
static bool is_changed_base_time;
static uint16_t time_cnt;
static time_state_e time_cnt_state;

// This is a counter for a tick.
volatile static uint8_t cnt_for_tick;
static uint8_t cnt_limit_for_tick;

static uint8_t cnt_for_reset;
static uint8_t cnt_for_changing_time_slow;
static uint8_t cnt_for_standby;
static uint16_t cnt_playing_pics_standby = TIME_STANDBY_START;

static bool is_first_button_plus_minus_pushed;

static void load_base_time(void) {
  base_time = eeprom_load(&base_time_addr);
}

static void store_base_time(void) {
  if (is_changed_base_time) {
    eeprom_store(&base_time_addr, base_time);
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

static void increment_time(void) {
  if (time_cnt_state != TIME_PAUSED) {
    if (base_time < MAX_TIME) {
      base_time++;
      is_changed_base_time = true;
    }
  } else {
    if (time_cnt < base_time) {
      time_cnt++;
    }
  }
}

static void decrement_time(void) {
  uint16_t* ptr = &time_cnt;
  if (time_cnt_state != TIME_PAUSED) {
    ptr = &base_time;
    is_changed_base_time = true;
  }
  if ((*ptr) > MIN_TIME) {
    (*ptr)--;
  }
}

static void increment_cnt_for_slow(void) {
  if (cnt_for_changing_time_slow < CNT_FOR_CHANGING_TIME_SLOW_PERIOD) {
    cnt_for_changing_time_slow++;
  }
}

static void do_action_at_tick(button_event_e pushed_button) {
  if (time_cnt_state == TIME_DECREASING) {
    decrement_time_cnt();
  } else if (time_cnt_state == TIME_END) {
    if (cnt_for_reset < (CNT_FOR_RESET - 1)) {
      cnt_for_reset++;
    } else {
      cnt_for_reset = 0;
      time_cnt_state = TIME_RESET;
    }
  } else if (pushed_button == BUTTON_EVENT_PLUS) {
    increment_time();
    increment_cnt_for_slow();
  } else if (pushed_button == BUTTON_EVENT_MINUS) {
    decrement_time();
    increment_cnt_for_slow();
  } else {
    if (time_cnt_state == TIME_STANDBY) {
      if (cnt_playing_pics_standby < TIME_STANDBY_END) {
	cnt_playing_pics_standby++;
      } else {
	cnt_playing_pics_standby = TIME_STANDBY_START;
      }
    }

    if (time_cnt_state == TIME_PAUSED ||
	time_cnt_state == TIME_RESET) {
      if (cnt_for_standby < (CNT_FOR_STANDBY - 1)) {
	cnt_for_standby++;
      } else {
	time_cnt = base_time;
	store_base_time();
	time_cnt_state = TIME_STANDBY;
	cnt_playing_pics_standby = TIME_STANDBY_START;
	cnt_limit_for_tick = CNT_LIMIT_PLAYING_STNDBY_PICTURES;
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
    cnt_limit_for_tick = CNT_LIMIT_COUNTDOWN;
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

  if (time_cnt_state == TIME_STANDBY) {
    time_cnt_state = TIME_RESET;
  }

  if (cnt_for_changing_time_slow < CNT_FOR_CHANGING_TIME_SLOW_PERIOD) {
    cnt_limit_for_tick = CNT_LIMIT_CHANGING_TIME_SLOW;
  } else {
    cnt_limit_for_tick = CNT_LIMIT_CHANGING_TIME_FAST;
  }

  if (!is_first_button_plus_minus_pushed) {
    if (pushed_button == BUTTON_EVENT_PLUS) {
      increment_time();
    } else {
      decrement_time();
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
    // wanted fall through
  case TIME_PAUSED:
    cnt_limit_for_tick = CNT_LIMIT_WAITING_STANDBY;
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

  cnt_for_changing_time_slow = 0;
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
    cnt_limit_for_tick = CNT_LIMIT_WAITING_AT_END;
    gpio_relay_reset();
  }

  if (evaluate_cnt_for_tick()) {
    do_action_at_tick(pushed_button);
  }
}

uint16_t time_get_for_display(void) {
  if (time_cnt_state == TIME_STANDBY) {
    return cnt_playing_pics_standby;
  }

  if (time_cnt_state != TIME_RESET) {
    return time_cnt;
  }

  return base_time;
}

uint8_t time_get_progress_in_pixels(uint8_t disp_pixels) {
  if (time_cnt_state == TIME_STANDBY || time_cnt_state == TIME_RESET) {
    return 0;
  }

  return (disp_pixels * (uint32_t)(base_time - time_cnt)) / base_time;
}
