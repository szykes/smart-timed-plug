#ifndef OLED_BITMAP_H_
#define OLED_BITMAP_H_

#include "oled_conf.h"

#include <stdint.h>

#define OLED_DIGIT_COLS (31u)
#define OLED_DIGIT_ROWS (6u)

#define OLED_DIGIT_BYTES (OLED_DIGIT_COLS * OLED_DIGIT_ROWS)

#define OLED_POINT_COLS (9u)
#define OLED_POINT_ROWS (2u)

extern const uint8_t bitmap_point[];
extern const uint8_t bitmap_digits[][OLED_DIGIT_BYTES];
extern const uint8_t bitmap_coffee[][OLED_DISP_BYTES];

#endif /* OLED_BITMAP_H_ */
