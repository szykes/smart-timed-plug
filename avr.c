#include "avr.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "button.h"
#include "time.h"

void hw_init(void) {
  // GPIO
  DDRA = (1 << DD3) | (1 << DD4 /* SCK */) | (1 << DD5 /* DO */) | (1 << DD7);
  PORTA = (1 << PORT0) | (1 << PORT1) | (1 << PORT2);
  DDRB = (1 << DD0) | (1 << DD1);

  // Timer 1
  // Calculate the OCR1A value for the desired interval
  // Formula: OCR1A = (F_CPU / (2 * Prescaler * (1 / Desired Frequency))) - 1
  // Desired Frequency = 1 / TIMER_INTERVAL_MS (converted to seconds)

  TCCR1B = (1 << WGM12 /* CTC */) | (1 << CS11) | (1 << CS10 /* prescaler is 64 */);

  OCR1A = (F_CPU / (64ul * 1000ul / TIMER_INTERRUPT_PERIOD_TIME)) - 1u;

  TIMSK1 = (1 << OCIE1A);

  // SPI
  USICR = (1 << USIWM0) | (0 << USIWM1) |  // Three-Wire Mode (SPI)
        (1 << USICS1) |                  // External Clock (USCK)
        (1 << USICLK);                   // Software Clock Strobe (Required for Master mode)

  // Watchdog
  wdt_enable(WDTO_250MS);
}

void mcu_sei(void) {
  sei();
}

void mcu_cli(void) {
  cli();
}

void eeprom_store(uint16_t *addr, uint16_t value) {
  eeprom_write_word(addr, value);
}

uint16_t eeprom_load(uint16_t *addr) {
  return eeprom_read_word(addr);
}

uint8_t gpio_inputs_get(void) {
  return (~PINA) & ((1 << PIN0) | (1 << PIN1) | (1 << PIN2));
}

void gpio_relay_set(void) {
  PORTB |= (1 << PORT0);
}

void gpio_relay_reset(void) {
  PORTB &= ~(1 << PORT0);
}

void gpio_oled_reset_set(void) {
  PORTA |= (1 << PORT3);
}

void gpio_oled_reset_reset(void) {
  PORTA &= ~(1 << PORT3);
}

void gpio_oled_dc_set(void) {
  PORTA |= (1 << PORT7);
}

void gpio_oled_dc_reset(void) {
  PORTA &= ~(1 << PORT7);
}

void gpio_oled_cs_set(void) {
  PORTB |= (1 << PORT1);
}

void gpio_oled_cs_reset(void) {
  PORTB &= ~(1 << PORT1);
}

ISR(TIM1_COMPA_vect, ISR_BLOCK) {
  button_interrupt();
  time_interrupt();
}

void spi_send_byte(uint8_t byte) {
  USIDR = byte;
  USISR = (1 << USIOIF);
  while ((USISR & (1 << USIOIF)) == 0) {
    USICR |= (1 << USITC);
  }
}

void wdt_restart(void) {
  wdt_reset();
}
