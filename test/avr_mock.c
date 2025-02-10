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
  MOCK_RECORD_1_PARAM_RET(size_t, addr, uint8_t);
}

void eeprom_store(size_t addr, uint8_t data) {
  MOCK_RECORD_2_PARAM(size_t, addr, uint8_t, data);
}

void gpio_init(void) {
  MOCK_RECORD();
}

uint8_t gpio_inputs_get(void) {
  MOCK_RECORD_RET(uint8_t);
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
  MOCK_RECORD_1_PARAM(uint8_t, byte);
}

void wdt_init(void) {
  MOCK_RECORD();
}

void wdt_restart(void) {
  MOCK_RECORD();
}
