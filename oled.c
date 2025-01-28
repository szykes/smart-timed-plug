#include "oled.h"

#include <stdint.h>

#include "avr.h"

// OLED is based on SSD1309

#define OLED_WIDTH (128u)
#define OLED_HEIGHT (64u)

#define OLED_COLS (OLED_WIDTH)
#define OLED_ROWS (OLED_HEIGHT / 8u)

#define CMD_SET_LOWER_COL_START_ADDR (0x00u) // + offset
#define CMD_SET_HIGHER_COL_START_ADDR (0X10u) // + offset
#define CMD_SET_DISPLAY_OFF (0xAEu)
#define CMD_SET_DISPLAY_ON (0xAFu)
#define CMD_SET_PAGE_ADDR (0xB0u) // + page address
#define CMD_SET_MEMORY_ADDRESSING_MODE (0x20u)
#define CMD_VAL_PAGE_ADDRESSING_MODE (0x02u)
#define CMD_SET_DISPLAY_START_LINE_0 (0x40u) // starts with row 0
#define CMD_SET_NORMAL_DISPLAY (0xA6u)
#define CMD_SET_V_COMH_DESELECT_LEVEL (0xDBu)
#define CMD_VAL_0_83_X_VCC (0x3Cu) // ~0.84xVCC  || (0x20); /* 0x20, 0.77xVcc */
#define CMD_ENTIRE_DISPLAY_ON_WITH_RAM (0xA4u)
#define CMD_SET_CONTRAST (0x81u)
#define CMD_VAL_CONTRAST_128 (128u) // max 256
#define CMD_SET_DISPLAY_CLOCK (0xD5u)
#define CMD_VAL_DISPLAY_CLOCK (0x70u) // 105 Hz
#define CMD_SET_SEGMNET_REMAP_NON_MIRRORED (0xA1u)
#define CMD_SET_COM_OUTPUT_SCAN_DIRECTION_NON_MIRRORED (0xC8u)
#define CMD_SET_COM_PIN_HW_CONFIG (0xDAu)
#define CMD_VAL_ALTERNATIVE_COM_PIN (0x12u)

static void write_command(uint8_t cmd) {
  gpio_oled_dc_reset();

}

/* static void write_data(uint8_t data) { */
/*   gpio_oled_dc_set(); */

/* } */

void oled_init(void) {
  write_command(CMD_SET_DISPLAY_OFF);

/*   Write_command(0xA8); // Select Multiplex Ratio */
/* Write_command(0x3F); // Default => 0x3F (1/64 Duty) 0x1F(1/32 Duty) */

/*   Write_command(0xD3); //Setting Display Offset */
/* Write_command(0x00); //00H Reset */

  write_command(CMD_SET_MEMORY_ADDRESSING_MODE);
  write_command(CMD_VAL_PAGE_ADDRESSING_MODE);

/*   Write_command(0x00); //Set Column Address LSB */
/* Write_command(0x10); //Set Column Address MSB */

  write_command(CMD_SET_DISPLAY_START_LINE_0);

  write_command(CMD_SET_NORMAL_DISPLAY);

  write_command(CMD_SET_V_COMH_DESELECT_LEVEL);
  write_command(CMD_VAL_0_83_X_VCC);

  write_command(CMD_ENTIRE_DISPLAY_ON_WITH_RAM);

  write_command(CMD_SET_SEGMNET_REMAP_NON_MIRRORED);

  write_command(CMD_SET_COM_OUTPUT_SCAN_DIRECTION_NON_MIRRORED);

  write_command(CMD_SET_COM_PIN_HW_CONFIG);
  write_command(CMD_VAL_ALTERNATIVE_COM_PIN);

/* Write_command(0xD9); //Set Pre-Charge period */
/* Write_command(0x22);   */

  write_command(CMD_SET_DISPLAY_ON);
}

void oled_update_screen(void) {
  for (uint8_t row = 0; row < OLED_ROWS; row++) {
    write_command(CMD_SET_PAGE_ADDR + row);
    // TODO: is this needed?
    /* write_command(CMD_SET_LOWER_COL_START_ADDR + OFFSET); */
    /* write_command(CMD_SET_HIGHER_COL_START_ADDR + OFFSET); */
    for (uint8_t col = 0; col < OLED_COLS; col++) {
    }
  }
}
