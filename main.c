#include "avr.h"

#include "avr.h"
#include "button.h"
#include "time.h"
#include "oled.h"

int main()
{
  mcu_sei();
  gpio_init();
  timer_init();
  spi_init();
  time_init();
  oled_init();
  wdt_init();

  while(1) {
    button_main();
    time_main();
    oled_main();
    wdt_restart();
  }
}
