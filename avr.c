#include "avr.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "button.h"
#include "time.h"

#define TIMER_INTERVAL_MS 5

void mcu_sei(void) {
  sei();
}

void mcu_cli(void) {
  cli();
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

  OCR1A = (F_CPU / (64l * 1000l / TIMER_INTERVAL_MS)) - 1;

  TIMSK |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  button_5ms_task();
  time_5ms_task();
}

void spi_init(void) {
  DDRB |= (1 << DD3 /* MOSI */) | (1 << DD5 /* SCK */);
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPHA) | (1 << CPOL);
}

void spi_send_byte(uint8_t byte) {
  while(!(SPSR & (1<<SPIF)));
  SPDR = byte;
}

void wdt_init(void) {
  wdt_enable(WDTO_250MS);
}

void wdt_restart(void) {
  wdt_reset();
}
