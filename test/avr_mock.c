#include "avr.h"

#include "framework.h"
#include "mock.h"

#include <stdio.h>

void mcu_sei(void) {
  mock_record(NULL, 0, NULL);
}

void mcu_cli(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_init(void) {
  mock_record(NULL, 0, NULL);
}

uint8_t gpio_inputs_get(void) {
  type_st ret;

  mock_record(NULL, 0, &ret);

  if (ret.type == TYPE_UINT8_T) {
    return *((uint8_t*)ret.value);
  }

  log_error("Invalid return type");
  return 0;
}

void gpio_relay_set(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_relay_reset(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_reset_set(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_reset_reset(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_dc_set(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_dc_reset(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_cs_set(void) {
  mock_record(NULL, 0, NULL);
}

void gpio_oled_cs_reset(void) {
  mock_record(NULL, 0, NULL);
}

void timer_init(void) {
  mock_record(NULL, 0, NULL);
}

void spi_init(void) {
  mock_record(NULL, 0, NULL);
}

void spi_send_byte(uint8_t byte) {
  /* type_st params[] = { */
  /*   { */
  /*     .type = TYPE_CHAR, */
  /*     .value = &data, */
  /*     .size = sizeof(data), */
  /*   } */
  /* }; */
  /* mock_record(params, sizeof(params)/sizeof(type_st), NULL); */

#define OLED_WIDTH (128u)
#define OLED_HEIGHT (64u)

#define OLED_COLS (OLED_WIDTH)
#define OLED_ROWS (OLED_HEIGHT / 8u)

  static size_t row;
  static size_t col;

  static uint8_t buff[OLED_ROWS][OLED_COLS];

  if (col >= OLED_COLS) {
    col = 0;
    row++;
  }

  if (row >= OLED_ROWS) {
    for (uint8_t line = 0; line < 8; line++) {
      for (row = 0; row < 8; row++) {
	for (col = 0; col < OLED_COLS; col++) {
	  uint8_t data = buff[line][col];
	  uint8_t bit = data & (1 << row);
	  printf("%s", (bit ? "#" : " "));
	}
	printf("|\n");
      }
    }
    row = 0;
    col = 0;
    memset(buff, 0, sizeof(buff));
    return;
  }

  buff[row][col] = byte;

  col++;
}

void wdt_init(void) {
  mock_record(NULL, 0, NULL);
}

void wdt_restart(void) {
  mock_record(NULL, 0, NULL);
}
