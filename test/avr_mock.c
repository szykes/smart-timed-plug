#include "avr.h"

#include "mock.h"

#include <stdio.h>

void mcu_sei(void) {
  MOCK_RECORD();
}

void mcu_cli(void) {
  MOCK_RECORD();
}

uint8_t eeprom_load(size_t addr) {
  MOCK_RECORD_1_PARAM_RET(TYPE_SIZE_T, addr, TYPE_UINT8_T, uint8_t);
}

void eeprom_store(size_t addr, uint8_t data) {
  MOCK_RECORD_2_PARAM(TYPE_SIZE_T, addr, TYPE_UINT8_T, data);
}

void gpio_init(void) {
  MOCK_RECORD();
}

uint8_t gpio_inputs_get(void) {
  MOCK_RECORD_RET(TYPE_UINT8_T, uint8_t);
}

void gpio_relay_set(void) {
  MOCK_RECORD();
}

void gpio_relay_reset(void) {
  MOCK_RECORD();
}

void gpio_oled_reset_set(void) {
  MOCK_RECORD();
}

void gpio_oled_reset_reset(void) {
  MOCK_RECORD();
}

void gpio_oled_dc_set(void) {
  MOCK_RECORD();
}

void gpio_oled_dc_reset(void) {
  MOCK_RECORD();
}

void gpio_oled_cs_set(void) {
  MOCK_RECORD();
}

void gpio_oled_cs_reset(void) {
  MOCK_RECORD();
}

void timer_init(void) {
  MOCK_RECORD();
}

void spi_init(void) {
  MOCK_RECORD();
}

void spi_send_byte(uint8_t byte) {
  MOCK_RECORD_1_PARAM(TYPE_UINT8_T, byte);
}
/* void spi_send_byte(uint8_t byte) { */
/*   /\* type_st params[] = { *\/ */
/*   /\*   { *\/ */
/*   /\*     .type = TYPE_CHAR, *\/ */
/*   /\*     .value = &data, *\/ */
/*   /\*     .size = sizeof(data), *\/ */
/*   /\*   } *\/ */
/*   /\* }; *\/ */
/*   /\* mock_record(params, sizeof(params)/sizeof(type_st), NULL); *\/ */

/* #define OLED_WIDTH (128u) */
/* #define OLED_HEIGHT (64u) */

/* #define OLED_COLS (OLED_WIDTH) */
/* #define OLED_ROWS (OLED_HEIGHT / 8u) */

/*   static size_t row; */
/*   static size_t col; */

/*   static uint8_t buff[OLED_ROWS][OLED_COLS]; */

/*   static bool is_skip = false; */

/*   if (!is_skip && (col % 128) == 0) { */
/*     is_skip = true; */
/*     return; */
/*   } */

/*   if (col >= OLED_COLS) { */
/*     col = 0; */
/*     row++; */
/*   } */

/*   if (row >= (OLED_ROWS - 1) && col >= (OLED_COLS - 1)) { */
/*     for (uint8_t line = 0; line < 8; line++) { */
/*       for (row = 0; row < 8; row++) { */
/* 	for (col = 0; col < OLED_COLS; col++) { */
/* 	  uint8_t data = buff[line][col]; */
/* 	  uint8_t bit = data & (1 << row); */
/* 	  printf("%s", (bit ? "#" : " ")); */
/* 	} */
/* 	printf("|\n"); */
/*       } */
/*     } */
/*     row = 0; */
/*     col = 0; */
/*     memset(buff, 0, sizeof(buff)); */
/*     return; */
/*   } */

/*   buff[row][col] = byte; */

/*   is_skip = false; */

/*   col++; */
/* } */

void wdt_init(void) {
  MOCK_RECORD();
}

void wdt_restart(void) {
  MOCK_RECORD();
}
