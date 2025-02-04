#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>

#define TIME_STANDBY_START (10000u)
#define TIME_STANDBY_END (10003u)

void time_init(void);

void time_interrupt(void);

void time_main(void);

uint16_t time_get_for_display(void);
uint8_t time_get_progress_in_pixels(uint8_t disp_pixels);

#endif /* TIME_H_ */
