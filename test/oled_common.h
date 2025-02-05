#ifndef TEST_OLED_COMMON_H_
#define TEST_OLED_COMMON_H_

#include <stdint.h>

void expect_write_command(uint8_t cmd, char *msg);
void expect_write_data(uint8_t byte, char *msg);
void expect_print_progress(uint8_t progress_in_pixels, uint8_t *bytes, char *msg);
void expect_print_time(uint8_t time, uint8_t *bytes, char *msg);

#endif /* TEST_OLED_COMMON_H_ */
