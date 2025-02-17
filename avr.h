#ifndef AVR_H_
#define AVR_H_

#include <stdint.h>
#include <string.h>

#define GPIO_BTN_START_STOP (1 << 0)
#define GPIO_BTN_PLUS (1 << 1)
#define GPIO_BTN_MINUS (1 << 2)

#define TIMER_INTERRUPT_PERIOD_TIME (5u) // 5 ms

void hw_init(void);

void mcu_sei(void);
void mcu_cli(void);

void eeprom_store(uint16_t *addr, uint16_t value);
uint16_t eeprom_load(uint16_t *addr);

uint8_t gpio_inputs_get(void);

void gpio_relay_set(void);
void gpio_relay_reset(void);

void gpio_oled_reset_set(void);
void gpio_oled_reset_reset(void);

void gpio_oled_dc_set(void);
void gpio_oled_dc_reset(void);

void gpio_oled_cs_set(void);
void gpio_oled_cs_reset(void);

void spi_send_byte(uint8_t byte);

void wdt_restart(void);

#endif // AVR_H_
