#include "avr.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "button.h"
#include "time.h"

void hw_init(void) {
  // GPIO
  DDRA = (1 << DD3) | (1 << DD4 /* SCK */) | (1 << DD5 /* DO */) | (1 << DD7);
  PORTA = (1 << PORT0) | (1 << PORT1) | (2 << PORT2);
  DDRB = (1 << DD0) | (1 << DD1);

  // Timer 1
  // Calculate the OCR1A value for the desired interval
  // Formula: OCR1A = (F_CPU / (2 * Prescaler * (1 / Desired Frequency))) - 1
  // Desired Frequency = 1 / TIMER_INTERVAL_MS (converted to seconds)

  TCCR1B = (1 << WGM12 /* CTC */) | (1 << CS11) | (1 << CS10 /* prescaler is 64 */);

  OCR1A = (F_CPU / (64l * 1000l / TIMER_INTERRUPT_PERIOD_TIME)) - 1;

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

uint8_t eeprom_load(size_t addr) {
  while(EECR & (1<<EEPE));
  EEAR = addr;
  EECR |= (1<<EERE);
  return EEDR;
}

void eeprom_store(size_t addr, uint8_t data) {
  while(EECR & (1<<EEPE));
  EECR = (0<<EEPM1) | (0<<EEPM0);
  EEAR = addr;
  EEDR = data;
  EECR |= (1<<EEMPE);
  EECR |= (1<<EEPE);
}

uint8_t gpio_inputs_get(void) {
  uint8_t input = PINA & ((1 << PIN0) | (1 << PIN1) | (PIN2));
  return ~input;
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
  PORTA |= ~(1 << PORT7);
}

void gpio_oled_cs_set(void) {
  PORTB |= (1 << PORT1);
}

void gpio_oled_cs_reset(void) {
  PORTB &= ~(1 << PORT1);
}

ISR(TIM1_COMPA_vect) {
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
