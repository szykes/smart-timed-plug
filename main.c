#include "avr.h"

#include "avr.h"
#include "button.h"
#include "time.h"
#include "oled.h"

int main()
{
  hw_init();
  time_init();
  oled_init();
  mcu_sei();

  while(1) {
    button_main();
    time_main();
    oled_main();
    wdt_restart();
  }
}
