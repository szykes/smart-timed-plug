#include "avr.h"

#include "avr.h"
#include "button.h"
#include "time.h"
#include "oled.h"

int main()
{
  mcu_sei();
  hw_init();
  time_init();
  oled_init();

  while(1) {
    button_main();
    time_main();
    oled_main();
    wdt_restart();
  }
}
