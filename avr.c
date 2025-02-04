#include "avr.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "button.h"
#include "time.h"

void mcu_sei(void) {
  sei();
}

void mcu_cli(void) {
  cli();
}

uint8_t eeprom_load(size_t addr) {
  while(EECR & (1 << EEWE));
  EEAR = addr;
  EECR |= (1 << EERE);
  return EEDR;
}

void eeprom_store(size_t addr, uint8_t data) {
  while(EECR & (1 << EEWE));
  EEAR = addr;
  EEDR = data;
  EECR |= (1 << EEMWE);
  EECR |= (1 << EEWE);
}

void gpio_init(void) {}

uint8_t gpio_inputs_get(void) {
  return 0;
}

void gpio_relay_set(void) {}

void gpio_relay_reset(void) {}

void gpio_oled_reset_set(void) {}

void gpio_oled_reset_reset(void) {}

void gpio_oled_dc_set(void) {}

void gpio_oled_dc_reset(void) {}

void gpio_oled_cs_set(void) {}

void gpio_oled_cs_reset(void) {}

void timer_init(void) {
  // Calculate the OCR1A value for the desired interval
  // Formula: OCR1A = (F_CPU / (2 * Prescaler * (1 / Desired Frequency))) - 1
  // Desired Frequency = 1 / TIMER_INTERVAL_MS (converted to seconds)

  TCCR1B |= (1 << WGM12 /* CTC */);
  TCCR1B |= (1 << CS11) | (1 << CS10 /* prescaler is 64 */);

  OCR1A = (F_CPU / (64l * 1000l / TIMER_INTERRUPT_PERIOD_TIME)) - 1;

  TIMSK |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  button_interrupt();
  time_interrupt();
}

void spi_init(void) {
  DDRB |= (1 << DD3 /* MOSI */) | (1 << DD5 /* SCK */);
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPHA) | (1 << CPOL);
}

void spi_send_byte(uint8_t byte) {
  SPDR = byte;
  while(!(SPSR & (1<<SPIF)));
}

void wdt_init(void) {
  wdt_enable(WDTO_250MS);
}

void wdt_restart(void) {
  wdt_reset();
}
