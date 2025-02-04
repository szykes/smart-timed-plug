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

#define DIGIT_COLS (31u)
#define DIGIT_ROWS (6u)

#define ALL_DIGITS_OFFSET_COLS (4u)
#define ALL_DIGITS_OFFSET_ROWS (1u)

#define POINT_COLS (9u)
#define POINT_ROWS (2u)

#define POINT_OFFSET_ROWS (ALL_DIGITS_OFFSET_ROWS + (DIGIT_ROWS - POINT_ROWS))

#define SPACE_BETWEEN_DIGITS (6u)

static uint16_t prev_time;

// https://javl.github.io/image2cpp/
// Config:
// - "Invert image colors" Tick
// - "Code output format" Arduino code
// - "Draw mode" Vertical - 1 bit per pixel

// 'point', 9x16px
const uint8_t epd_bitmap_point[] = {
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x3e, 0x7f, 0x7f, 0xff, 0xff, 0xff, 0x7f,
	0x7f, 0x1c
};

// '0', 31x48px
const uint8_t epd_bitmap_0 [] = {
	0x00, 0x00, 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfc, 0xfe, 0x7e, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3e, 0x7e, 0xfe, 0xfc, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe0, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xe0, 0x00, 0xfc, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x0f, 0x3f, 0xff,
	0xff, 0xff, 0xff, 0xfc, 0xe0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0xe0, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03,
	0x0f, 0x1f, 0x1f, 0x3f, 0x7f, 0x7f, 0x7e, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 0xfc, 0xfc, 0x7e, 0x7f,
	0x7f, 0x3f, 0x1f, 0x1f, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00
};

static void write_command(uint8_t cmd) {
  gpio_oled_dc_reset();
  spi_send_byte(cmd);
}

static void write_data(uint8_t data) {
  /* gpio_oled_dc_set(); */
  spi_send_byte(data);
}

static uint8_t calculate_digit(uint16_t time, uint8_t pos) {
  uint8_t digit = 0;

  switch (pos) {
  case 0:
    digit = time % 10;
    break;

  case 1:
    digit = (time / 10) % 10;
    break;

  case 2:
    digit = (time / 100) % 10;
    break;

  default:
    break;
  }

  return digit;
}

static const uint8_t *get_digit_ptr(uint8_t digit) {
  return epd_bitmap_0;
}

static bool print_digit(const uint8_t *digit, uint8_t virt_row, uint8_t virt_col,
			uint8_t offset_rows, uint8_t offset_cols) {
  if (offset_cols <= virt_col && virt_col < (DIGIT_COLS + offset_cols) &&
      offset_rows <= virt_row && virt_row < (DIGIT_ROWS + offset_rows)) {
    uint16_t i = (virt_row - offset_rows) * DIGIT_COLS + (virt_col - offset_cols);
    write_data(digit[i]);
    return true;
  }

  return false;
}

static bool print_point(uint8_t virt_row, uint8_t virt_col,
                        uint8_t offset_rows, uint8_t offset_cols) {
  if (offset_cols <= virt_col && virt_col < (POINT_COLS + offset_cols) &&
      offset_rows <= virt_row && virt_row < (POINT_ROWS + offset_rows)) {
    uint16_t i = (virt_row - offset_rows) * POINT_COLS + (virt_col - offset_cols);
    write_data(epd_bitmap_point[i]);
    return true;
  }

  return false;
}

static void print_display(uint16_t time) {
  uint8_t first_digit = calculate_digit(time, 2);
  uint8_t second_digit = calculate_digit(time, 1);
  uint8_t third_digit = calculate_digit(time, 0);

  const uint8_t *first_digit_ptr = get_digit_ptr(first_digit);
  const uint8_t *second_digit_ptr = get_digit_ptr(second_digit);
  const uint8_t *third_digit_ptr = get_digit_ptr(third_digit);

  for (uint16_t i = 0; i < (OLED_COLS * OLED_ROWS); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if (virt_col == 0) {
      write_command(CMD_SET_PAGE_ADDR + virt_row);
    }

    if (!print_digit(first_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS) &&
	!print_digit(second_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + DIGIT_COLS + SPACE_BETWEEN_DIGITS) &&
	!print_point(virt_row, virt_col, POINT_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + 2 * (DIGIT_COLS + SPACE_BETWEEN_DIGITS)) &&
	!print_digit(third_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + (2 * DIGIT_COLS) + POINT_COLS + (3 * SPACE_BETWEEN_DIGITS))) {
      write_data(0x00);
    }
  }
}

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
    print_display(curr_time);
  }
}

void oled_update_screen(void) {
  print_display(000);
}
