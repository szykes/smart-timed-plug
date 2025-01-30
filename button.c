#include "button.h"

#include "avr.h"

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCING_CNT ((10u / TIMER_INTERRUPT_PERIOD_TIME) - 1)
#define LONG_CNT ((1000u / TIMER_INTERRUPT_PERIOD_TIME) - 2)

// When the button is pushed, the logic sends the PUSHED event immediately.
// There is debouncing right after and then the logic checks for button state.
typedef enum {
  BUTTON_STATE_RELEASED = 0,
  BUTTON_STATE_SHORT_PUSHED = 1,
  BUTTON_STATE_LONG_PUSHED = 2,
  BUTTON_STATE_DEBOUNCING = 3,
  BUTTON_STATE_WAITING = 4,
} button_state_e;

typedef struct {
  button_state_e state;
  uint8_t debouncing_cnt;
  uint8_t long_cnt;
} button_defer_st;

typedef struct {
  button_state_e state;
  uint8_t debouncing_cnt;
} button_eager_st;

static button_defer_st button_start_stop;
static button_eager_st button_plus;
static button_eager_st button_minus;

volatile static bool is_ticking;

static void evaluate_button_defer(bool raw_state) {
  switch (button_start_stop.state) {
  case BUTTON_STATE_RELEASED:
    if (raw_state) {
      button_start_stop.state = BUTTON_STATE_DEBOUNCING;
    }
    button_start_stop.long_cnt = DEBOUNCING_CNT + 1;
    break;

  case BUTTON_STATE_DEBOUNCING:
    if (button_start_stop.debouncing_cnt < DEBOUNCING_CNT) {
      button_start_stop.debouncing_cnt++;
    } else {
      if (raw_state) {
	button_start_stop.state = BUTTON_STATE_WAITING;
      } else {
	button_start_stop.state = BUTTON_STATE_SHORT_PUSHED;
      }
      button_start_stop.debouncing_cnt = 0;
    }
    break;

  case BUTTON_STATE_WAITING:
    if (raw_state) {
      if (button_start_stop.long_cnt < LONG_CNT) {
	button_start_stop.long_cnt++;
      } else {
	button_start_stop.state = BUTTON_STATE_LONG_PUSHED;
      }
    } else {
      button_start_stop.state = BUTTON_STATE_SHORT_PUSHED;
    }
    break;

  case BUTTON_STATE_SHORT_PUSHED:
  case BUTTON_STATE_LONG_PUSHED:
    button_start_stop.state = BUTTON_STATE_RELEASED;
    break;

  default:
    break;
  }
}

static void evaluate_button_eager(bool raw_state, button_eager_st *button) {
  switch (button->state) {
  case BUTTON_STATE_RELEASED:
    if (raw_state) {
      button->state = BUTTON_STATE_DEBOUNCING;
    }
    break;

  case BUTTON_STATE_DEBOUNCING:
    if (button->debouncing_cnt < DEBOUNCING_CNT) {
      button->debouncing_cnt++;
    } else {
      if (raw_state) {
	button->state = BUTTON_STATE_SHORT_PUSHED;
      } else {
	button->state = BUTTON_STATE_RELEASED;
      }
      button->debouncing_cnt = 0;
    }
    break;

  case BUTTON_STATE_SHORT_PUSHED:
    if (!raw_state) {
      button->state = BUTTON_STATE_RELEASED;
    }
    break;

  default:
    break;
  }
}

button_event_e button_is_pushed(void) {
  if (button_plus.state != BUTTON_STATE_RELEASED) {
    return BUTTON_EVENT_PLUS;
  }

  if (button_minus.state != BUTTON_STATE_RELEASED) {
    return BUTTON_EVENT_MINUS;
  }

  if (button_start_stop.state == BUTTON_STATE_SHORT_PUSHED) {
    return BUTTON_EVENT_START_STOP_SHORT;
  }
  if (button_start_stop.state == BUTTON_STATE_LONG_PUSHED) {
    return BUTTON_EVENT_START_STOP_LONG;
  }

  return BUTTON_EVENT_RELEASED;
}

void button_interrupt(void) {
  is_ticking = true;
}

void button_main(void) {
  mcu_cli();
  bool local_is_ticking = is_ticking;
  is_ticking = false;
  mcu_sei();

  if (!local_is_ticking) {
    return;
  }

  uint8_t button_raw = gpio_inputs_get();

  if ((button_raw & GPIO_BTN_START_STOP && (button_raw & (~GPIO_BTN_START_STOP)) != 0) ||
      (button_raw & GPIO_BTN_PLUS && (button_raw & (~GPIO_BTN_PLUS)) != 0) ||
      (button_raw & GPIO_BTN_MINUS && (button_raw & (~GPIO_BTN_MINUS)) != 0)) {
    button_start_stop.state = BUTTON_STATE_RELEASED;
    button_plus.state = BUTTON_STATE_RELEASED;
    button_minus.state = BUTTON_STATE_RELEASED;
    return;
  }

  evaluate_button_defer((button_raw & GPIO_BTN_START_STOP));
  evaluate_button_eager((button_raw & GPIO_BTN_PLUS), &button_plus);
  evaluate_button_eager((button_raw & GPIO_BTN_MINUS), &button_minus);
}
