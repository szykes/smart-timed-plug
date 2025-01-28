#include "button.h"

#include "avr.h"

#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCING_TIME_10_MS ((10u / TIMER_INTERRUPT_PERIOD_TIME) - 1)

// When the button is pushed, the logic sends the PUSHED event immediately.
// There is debouncing right after and then the logic checks for button state.
typedef enum {
  BUTTON_STATE_RELEASED = 0,
  BUTTON_STATE_PUSHED = 1,
  BUTTON_STATE_DEBOUNCING = 2,
} button_state_e;

typedef struct {
  button_state_e state;
  uint8_t debouncing_cnt;
} button_st;

button_st buttons[BUTTON_MAX];

volatile uint8_t button_raw;
volatile bool is_ticking;

void evaluate_button_state(bool raw_state, button_st *button) {
  switch (button->state) {
  case BUTTON_STATE_RELEASED:
    if (raw_state) {
      button->state = BUTTON_STATE_DEBOUNCING;
    }
    break;

  case BUTTON_STATE_DEBOUNCING:
    if (button->debouncing_cnt < DEBOUNCING_TIME_10_MS) {
      button->debouncing_cnt++;
    } else {
      button->state = BUTTON_STATE_PUSHED;
      button->debouncing_cnt = 0;
    }
  case BUTTON_STATE_PUSHED:
    if (!raw_state) {
      button->state = BUTTON_STATE_RELEASED;
    }
  }
}

button_e button_is_pushed(void) {
  for (uint8_t i = 0; i < BUTTON_MAX; i++) {
    if (buttons[i].state != BUTTON_STATE_RELEASED) {
      return i;
    }
  }

  return BUTTON_RELEASED;
}

void button_5ms_task(void) {
  button_raw = gpio_inputs_get();
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

  if (((button_raw & (~GPIO_BTN_START_STOP)) != 0) ||
      ((button_raw & (~GPIO_BTN_PLUS)) != 0) ||
      ((button_raw & (~GPIO_BTN_MINUS)) != 0)) {
    buttons[BUTTON_START_STOP].state = BUTTON_STATE_RELEASED;
    buttons[BUTTON_PLUS].state = BUTTON_STATE_RELEASED;
    buttons[BUTTON_MINUS].state = BUTTON_STATE_RELEASED;
    return;
  }

  evaluate_button_state((button_raw & GPIO_BTN_START_STOP), &buttons[BUTTON_START_STOP]);
  evaluate_button_state((button_raw & GPIO_BTN_PLUS), &buttons[BUTTON_PLUS]);
  evaluate_button_state((button_raw & GPIO_BTN_MINUS), &buttons[BUTTON_MINUS]);
}
