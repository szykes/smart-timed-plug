#include "oled.h"

#include "avr.h"
#include "time.h"

#include <stdint.h>
#include <stdbool.h>

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

#define DIGIT_COLS (29u)
#define DIGIT_ROWS (6u)

#define ALL_DIGITS_OFFSET_COLS (1u)
#define ALL_DIGITS_OFFSET_ROWS (1u)

#define SPACE_BETWEEN_DIGITS (2u)

static uint16_t prev_time;

static void write_command(uint8_t cmd) {
  gpio_oled_dc_reset();

}

/* static void write_data(uint8_t data) { */
/*   gpio_oled_dc_set(); */

/* } */

void oled_init(void) {
  uint8_t init_seq[] = {
    CMD_SET_DISPLAY_OFF,
    CMD_SET_MEMORY_ADDRESSING_MODE, CMD_VAL_PAGE_ADDRESSING_MODE,
    CMD_SET_DISPLAY_START_LINE_0,
    CMD_SET_NORMAL_DISPLAY,
    CMD_SET_V_COMH_DESELECT_LEVEL, CMD_VAL_0_83_X_VCC,
    CMD_ENTIRE_DISPLAY_ON_WITH_RAM,
    CMD_SET_SEGMNET_REMAP_NON_MIRRORED,
    CMD_SET_SEGMNET_REMAP_NON_MIRRORED,
    CMD_SET_COM_OUTPUT_SCAN_DIRECTION_NON_MIRRORED,
    CMD_SET_COM_PIN_HW_CONFIG, CMD_VAL_ALTERNATIVE_COM_PIN,
    CMD_SET_DISPLAY_ON,
  };

  for (uint8_t i = 0; i < sizeof(init_seq); i++) {
    write_command(init_seq[i]);
  }
/*   write_command(CMD_SET_DISPLAY_OFF); */

/* /\*   Write_command(0xA8); // Select Multiplex Ratio *\/ */
/* /\* Write_command(0x3F); // Default => 0x3F (1/64 Duty) 0x1F(1/32 Duty) *\/ */

/* /\*   Write_command(0xD3); //Setting Display Offset *\/ */
/* /\* Write_command(0x00); //00H Reset *\/ */

/*   write_command(CMD_SET_MEMORY_ADDRESSING_MODE); */
/*   write_command(CMD_VAL_PAGE_ADDRESSING_MODE); */

/* /\*   Write_command(0x00); //Set Column Address LSB *\/ */
/* /\* Write_command(0x10); //Set Column Address MSB *\/ */

/*   write_command(CMD_SET_DISPLAY_START_LINE_0); */

/*   write_command(CMD_SET_NORMAL_DISPLAY); */

/*   write_command(CMD_SET_V_COMH_DESELECT_LEVEL); */
/*   write_command(CMD_VAL_0_83_X_VCC); */

/*   write_command(CMD_ENTIRE_DISPLAY_ON_WITH_RAM); */

/*   write_command(CMD_SET_SEGMNET_REMAP_NON_MIRRORED); */

/*   write_command(CMD_SET_COM_OUTPUT_SCAN_DIRECTION_NON_MIRRORED); */

/*   write_command(CMD_SET_COM_PIN_HW_CONFIG); */
/*   write_command(CMD_VAL_ALTERNATIVE_COM_PIN); */

/* /\* Write_command(0xD9); //Set Pre-Charge period *\/ */
/* /\* Write_command(0x22);   *\/ */

/*   write_command(CMD_SET_DISPLAY_ON); */
}

void oled_main(void) {
  uint16_t curr_time = time_get_for_display();

  if (prev_time != curr_time) {

  }
}

// https://javl.github.io/image2cpp/
// Config:
// - "Invert image colors" Tick
// - "Code output format" Arduino code
// - "Draw mode" Vertical - 1 bit per pixel

// '0', 29x45px
const uint8_t epd_bitmap_0 [] = {
  	0x00, 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0x7e, 0x3e, 0x3f, 0x1f, 0x1f, 0x1f, 0x1f,
	0x1f, 0x3f, 0x7e, 0xfe, 0xfc, 0xfc, 0xf8, 0xf0, 0xe0, 0x80, 0x00, 0x00, 0x00, 0x80, 0xf8, 0xff,
	0xff, 0xff, 0xff, 0x7f, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xf0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x1f, 0x00, 0x03, 0x1f, 0x7f, 0xff, 0xff, 0xff, 0xfc, 0xf0, 0xe0, 0xc0, 0x80,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xfe, 0xff, 0xff, 0xff, 0x3f, 0x0f, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0f, 0x0f, 0x0f, 0x1f, 0x1f, 0x1f, 0x1f,
	0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

bool print_digit(const uint8_t *digit, uint8_t virt_row, uint8_t virt_col, uint8_t offset_rows, uint8_t offset_cols) {
  if (offset_cols <= virt_col && virt_col < (DIGIT_COLS + offset_cols) &&
      offset_rows <= virt_row && virt_row < (DIGIT_ROWS + offset_rows)) {
    uint16_t i = (virt_row - offset_rows) * DIGIT_COLS + (virt_col - offset_cols);
    spi_send_byte(digit[i]);
    return true;
  }

  return false;
}

void oled_update_screen(void) {
  for (uint16_t i = 0; i < ((OLED_COLS * OLED_ROWS) + 1); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if (!print_digit(epd_bitmap_0, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS, ALL_DIGITS_OFFSET_COLS) &&
	!print_digit(epd_bitmap_0, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS, ALL_DIGITS_OFFSET_COLS + DIGIT_COLS + SPACE_BETWEEN_DIGITS) &&
	!print_digit(epd_bitmap_0, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS, ALL_DIGITS_OFFSET_COLS + 2 * (DIGIT_COLS + SPACE_BETWEEN_DIGITS))) {
      spi_send_byte(0x00);
    }
  }
}
