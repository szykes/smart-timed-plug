#include "time.h"

#include "mock.h"

uint16_t time_get_for_display(void) {
  MOCK_RECORD_RET(TYPE_UINT16_T, uint16_t);
}

uint8_t time_get_progress_in_pixels(uint8_t disp_pixels) {
  MOCK_RECORD_1_PARAM_RET(TYPE_UINT8_T, disp_pixels, TYPE_UINT8_T, uint8_t);
}
