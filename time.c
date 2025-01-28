#include "time.h"

#include "avr.h"
#include "button.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_TIME (999u)

#define CNT_LIMIT_100_MS (100u / TIMER_INTERRUPT_PERIOD_TIME)
#define CNT_LIMIT_500_MS (500u / TIMER_INTERRUPT_PERIOD_TIME)

typedef enum {
  TIME_RESET = 0,
  TIME_START = 1,
  TIME_DECREASING = 2,
  TIME_STOP = 3,
} time_state_e;

static uint16_t time_set = 150;
static uint16_t time_cnt;
static time_state_e time_cnt_state;

volatile static uint8_t wait_cnt;
static uint8_t wait_cnt_limit;

static bool evaluate_wait_cnt(void) {
  bool is_ticking = false;

  mcu_cli();
  if (wait_cnt_limit > 0) {
    if (wait_cnt == (wait_cnt_limit - 1)) {
      is_ticking = true;
    }
  } else {
    wait_cnt = 0;
  }
  mcu_sei();

  return is_ticking;
}

static void handle_button_start_stop(void) {
  switch (time_cnt_state) {
  case TIME_RESET:
  case TIME_STOP:
    time_cnt_state = TIME_START;
    wait_cnt_limit = CNT_LIMIT_100_MS;
    gpio_relay_set();
    break;

  case TIME_DECREASING:
    time_cnt_state = TIME_STOP;
    wait_cnt_limit = 0;
    gpio_relay_reset();
    break;

  default:
    break;
  }
}

static void handle_button_released(void) {
  switch (time_cnt_state) {
  case TIME_RESET:
    time_cnt = time_set;
    break;

  case TIME_START:
    time_cnt_state = TIME_DECREASING;
    break;

  case TIME_DECREASING:
    if (time_cnt == 0) {
      time_cnt_state = TIME_RESET;
      wait_cnt_limit = 0;
      gpio_relay_reset();
    }
    break;

  default:
    break;
  }
}

void time_5ms_task(void) {
  wait_cnt++;
}

void time_main(void) {
  button_e pushed_button = button_is_pushed();
  switch (pushed_button) {
  case BUTTON_START_STOP:
    handle_button_start_stop();
    break;

  case BUTTON_PLUS:
  case BUTTON_MINUS:
    wait_cnt_limit = CNT_LIMIT_500_MS;
    break;

  case BUTTON_RELEASED:
    handle_button_released();
    break;
  }

  if (evaluate_wait_cnt()) {
    if (time_cnt_state == TIME_DECREASING) {
      if (time_cnt > 0) {
	time_cnt--;
      }
    } else if (pushed_button == BUTTON_PLUS) {
      if (time_set < MAX_TIME) {
	time_set++;
      }
    } else if (pushed_button == BUTTON_MINUS) {
      if (time_set > 0) {
	time_set--;
      }
    }
  }
}
