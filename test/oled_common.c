#include "oled_common.h"

#include "mock.h"

#define OLED_COLS (128u)
#define OLED_ROWS (8u)

#define CMD_SET_PAGE_ADDR (0xB0u) // + page address

void expect_write_command(uint8_t cmd, char *msg) {
  MOCK_EXPECT("gpio_oled_dc_reset", msg);
  MOCK_EXPECT_1_PARAM("spi_send_byte", TYPE_UINT8_T, cmd, msg);
}

void expect_write_data(uint8_t byte, char *msg) {
  MOCK_EXPECT("gpio_oled_dc_set", msg);
  MOCK_EXPECT_1_PARAM("spi_send_byte", TYPE_UINT8_T, byte, msg);
}

void expect_print_progress(uint8_t progress_in_pixels, uint8_t *bytes, char *msg) {
  expect_write_command(CMD_SET_PAGE_ADDR + 0, msg);

  uint8_t cols = OLED_COLS;
  MOCK_EXPECT_1_PARAM_RET("time_get_progress_in_pixels", TYPE_UINT8_T, cols, TYPE_UINT8_T, progress_in_pixels, msg);

  for (size_t i = 0; i < OLED_COLS; i++) {
    expect_write_data(bytes[i], msg);
  }
}

void expect_print_time(uint8_t time, uint8_t *bytes, char *msg) {
  for (size_t i = OLED_COLS; i < (OLED_COLS * OLED_ROWS); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if (virt_col == 0) {
      expect_write_command(CMD_SET_PAGE_ADDR + virt_row, msg);
    }
    expect_write_data(bytes[i], msg);
  }
}
