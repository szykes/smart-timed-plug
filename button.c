#include "button.h"

#include "avr.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  RELEASED = 0,
  PUSHED = 1,
  DEBOUNCING = 2,
} button_state_e;

typedef struct {
  button_state_e state;
  uint8_t debounce_cnt;
} button_st;

button_st buttons[BUTTON_MAX];

volatile uint8_t button_raw;
volatile bool is_new_state;

void evaluate_button_state(bool raw_state, button_st *button) {
  switch (button->state) {
  case RELEASED:
    if (raw_state) {
      button->state = DEBOUNCING;
    }
    break;

  case DEBOUNCING:
    if (button->debounce_cnt < 1) {
      button->debounce_cnt++;
    } else {
      button->state = PUSHED;
      button->debounce_cnt = 0;
    }
  case PUSHED:
    if (!raw_state) {
      button->state = RELEASED;
    }
  }
}

button_e button_is_pushed(void) {
  for (uint8_t i = 0; i < BUTTON_MAX; i++) {
    if (buttons[i].state != RELEASED) {
      return i;
    }
  }

  return BUTTON_RELEASED;
}

void button_5ms_task(void) {
  button_raw = gpio_inputs_get();
  is_new_state = true;
}

void button_main(void) {
  mcu_cli();
  if (is_new_state) {
    is_new_state = false;
    mcu_sei();

    if (((button_raw & (~GPIO_BTN_START_STOP)) != 0) ||
	((button_raw & (~GPIO_BTN_PLUS)) != 0) ||
	((button_raw & (~GPIO_BTN_MINUS)) != 0)) {
      buttons[BUTTON_START_STOP].state = RELEASED;
      buttons[BUTTON_PLUS].state = RELEASED;
      buttons[BUTTON_MINUS].state = RELEASED;
      return;
    }

    evaluate_button_state((button_raw & GPIO_BTN_START_STOP), &buttons[BUTTON_START_STOP]);
    evaluate_button_state((button_raw & GPIO_BTN_PLUS), &buttons[BUTTON_PLUS]);
    evaluate_button_state((button_raw & GPIO_BTN_MINUS), &buttons[BUTTON_MINUS]);
  }
  mcu_sei();
}
