#include "oled.h"

#include "framework.h"

#include <stdint.h>

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

static bool tc_init(void) {
  TEST_BEGIN();

  uint8_t init_seq[] = {
    CMD_SET_DISPLAY_OFF,
    CMD_SET_MEMORY_ADDRESSING_MODE, CMD_VAL_PAGE_ADDRESSING_MODE,
    CMD_SET_DISPLAY_START_LINE_0,
    CMD_SET_NORMAL_DISPLAY,
    CMD_SET_V_COMH_DESELECT_LEVEL, CMD_VAL_0_83_X_VCC,
    CMD_ENTIRE_DISPLAY_ON_WITH_RAM,
    CMD_SET_SEGMNET_REMAP_NON_MIRRORED,
    CMD_SET_COM_OUTPUT_SCAN_DIRECTION_NON_MIRRORED,
    CMD_SET_COM_PIN_HW_CONFIG, CMD_VAL_ALTERNATIVE_COM_PIN,
    CMD_SET_DISPLAY_ON,
  };

  MOCK_EXPECT("gpio_oled_reset_reset", "");
  MOCK_EXPECT("gpio_relay_set", "");

  for (uint8_t i = 0; i < sizeof(init_seq); i++) {
    MOCK_EXPECT("gpio_oled_dc_reset", "");
    MOCK_EXPECT_1_PARAM("spi_send_byte", TYPE_UINT8_T, init_seq[i], "");
  }

  oled_init();

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tc_init());
  TEST_EVALUATE_END();
}
