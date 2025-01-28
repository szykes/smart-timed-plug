#ifndef BUTTON_H_
#define BUTTON_H_

typedef enum {
  BUTTON_START_STOP = 0,
  BUTTON_PLUS = 1,
  BUTTON_MINUS = 2,
  BUTTON_MAX = 3,
  BUTTON_RELEASED = BUTTON_MAX,
} button_e;

button_e button_is_pushed(void);

void button_5ms_task(void);

void button_main(void);

#endif /* BUTTON_H_ */
