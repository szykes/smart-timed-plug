#ifndef BUTTON_H_
#define BUTTON_H_

typedef enum {
  BUTTON_EVENT_RELEASED = 0,
  BUTTON_EVENT_START_STOP_SHORT = 1,
  BUTTON_EVENT_START_STOP_LONG = 2,
  BUTTON_EVENT_PLUS = 3,
  BUTTON_EVENT_MINUS = 4,
} button_event_e;

button_event_e button_is_pushed(void);

void button_interrupt(void);

void button_main(void);

#endif /* BUTTON_H_ */
