#include "oled_conf.h"
#include "oled_common.h"

#include "mock.h"

#define CMD_SET_PAGE_ADDR (0xB0u) // + page address

void expect_write_command(uint8_t cmd, char *msg) {
  MOCK_EXPECT("gpio_oled_dc_reset", msg);
  MOCK_EXPECT_1_PARAM("spi_send_byte", uint8_t, cmd, msg);
}

void expect_write_data(uint8_t byte, char *msg) {
  MOCK_EXPECT("gpio_oled_dc_set", msg);
  MOCK_EXPECT_1_PARAM("spi_send_byte", uint8_t, byte, msg);
}

void expect_print_progress(uint8_t progress_in_pixels, uint8_t *bytes, char *msg) {
  expect_write_command(CMD_SET_PAGE_ADDR + 0, msg);

  uint8_t cols = OLED_COLS;
  MOCK_EXPECT_1_PARAM_RET("time_get_progress_in_pixels", uint8_t, cols, uint8_t, progress_in_pixels, msg);

  for (size_t i = 0; i < OLED_COLS; i++) {
    expect_write_data(bytes[i], msg);
  }
}

void expect_print_time(const uint8_t *bytes, char *msg) {
  for (size_t i = OLED_COLS; i < (OLED_COLS * OLED_ROWS); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if (virt_col == 0) {
      expect_write_command(CMD_SET_PAGE_ADDR + virt_row, msg);
    }
    expect_write_data(bytes[i], msg);
  }
}

void expect_print_coffee(const uint8_t *bytes, char *msg) {
  for (size_t i = 0; i < (OLED_COLS * OLED_ROWS); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if (virt_col == 0) {
      expect_write_command(CMD_SET_PAGE_ADDR + virt_row, msg);
    }
    expect_write_data(bytes[i], msg);
  }
}
