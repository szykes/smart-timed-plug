#include "oled.h"

#include "oled_common.h"
#include "oled_progress_gen.h"
#include "oled_time_gen.h"

#include "mock.h"
#include "framework.h"

#include <stdio.h>

static bool tcs_all_progress(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(progress_tcs)/sizeof(progress_tcs[0]); i++) {
    oled_gen_progress_st tc = progress_tcs[i];
    char msg[50];
    snprintf(msg, sizeof(msg), "progress_in_pixels: %lu\n", i);
    uint16_t time = tc.progress_in_pixels;
    MOCK_EXPECT_RET("time_get_for_display", TYPE_UINT16_T, time, msg);

    expect_print_progress(tc.progress_in_pixels, tc.disp_data, msg);
    expect_print_time(tc.disp_data, msg);

    oled_main();

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

static bool tcs_all_time(void) {
  TEST_BEGIN();

  for (size_t i = 0; i < sizeof(time_tcs)/sizeof(time_tcs[0]); i++) {
    oled_gen_time_st tc = time_tcs[i];
    char msg[50];
    snprintf(msg, sizeof(msg), "time: %03lu\n", i);
    MOCK_EXPECT_RET("time_get_for_display", TYPE_UINT16_T, tc.time, msg);

    expect_print_progress(0, tc.disp_data, msg);
    expect_print_time(tc.disp_data, msg);

    oled_main();

    TEST_CHECK_MOCK();
  }

  TEST_END();
}

int main(void) {
  TEST_EVALUATE_INIT();
  TEST_EVALUATE(tcs_all_progress());
  TEST_EVALUATE(tcs_all_time());
  TEST_EVALUATE_END();
}
