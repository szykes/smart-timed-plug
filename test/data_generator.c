#include "oled_conf.h"
#include "oled_bitmap.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_TIME (999u)
#define MIN_TIME (1u)

#define ALL_DIGITS_OFFSET_COLS (4u)
#define ALL_DIGITS_OFFSET_ROWS (1u)

#define POINT_OFFSET_ROWS (ALL_DIGITS_OFFSET_ROWS + (OLED_DIGIT_ROWS - OLED_POINT_ROWS))

#define SPACE_BETWEEN_DIGITS (6u)

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
  return (const uint8_t *)&bitmap_digits[digit];
}

static bool print_digit(uint8_t *bytes, const uint8_t *digit, uint8_t virt_row, uint8_t virt_col,
			uint8_t offset_rows, uint8_t offset_cols) {
  if (offset_cols <= virt_col && virt_col < (OLED_DIGIT_COLS + offset_cols) &&
      offset_rows <= virt_row && virt_row < (OLED_DIGIT_ROWS + offset_rows)) {
    uint16_t i = (virt_row - offset_rows) * OLED_DIGIT_COLS + (virt_col - offset_cols);
    uint8_t byte = digit[i];
    *bytes = byte;
    return true;
  }

  return false;
}

static bool print_point(uint8_t *bytes, uint8_t virt_row, uint8_t virt_col,
                        uint8_t offset_rows, uint8_t offset_cols) {
  if (offset_cols <= virt_col && virt_col < (OLED_POINT_COLS + offset_cols) &&
      offset_rows <= virt_row && virt_row < (OLED_POINT_ROWS + offset_rows)) {
    uint16_t i = (virt_row - offset_rows) * OLED_POINT_COLS + (virt_col - offset_cols);
    *bytes = bitmap_point[i];
    return true;
  }

  return false;
}

static void print_progress(uint8_t *bytes, uint8_t progress) {
  for (uint8_t i = 0; i < OLED_COLS; i++) {
    if (i < progress) {
      bytes[i] = 0x0F;
    } else {
      bytes[i] = 0x00;
    }
  }
}

static void fill_bytes(uint8_t *bytes, uint16_t time, uint8_t progress) {
  uint8_t first_digit = calculate_digit(time, 2);
  uint8_t second_digit = calculate_digit(time, 1);
  uint8_t third_digit = calculate_digit(time, 0);

  const uint8_t *first_digit_ptr = get_digit_ptr(first_digit);
  const uint8_t *second_digit_ptr = get_digit_ptr(second_digit);
  const uint8_t *third_digit_ptr = get_digit_ptr(third_digit);

  print_progress(bytes, progress);

  for (uint16_t i = OLED_COLS; i < (OLED_COLS * OLED_ROWS); i++) {
    uint8_t virt_row = i / OLED_COLS;
    uint8_t virt_col = i % OLED_COLS;

    if ((first_digit == 0 ||
	 !print_digit(&bytes[i], first_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		      ALL_DIGITS_OFFSET_COLS)) &&
	!print_digit(&bytes[i], second_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + OLED_DIGIT_COLS + SPACE_BETWEEN_DIGITS) &&
	!print_point(&bytes[i], virt_row, virt_col, POINT_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + 2 * (OLED_DIGIT_COLS + SPACE_BETWEEN_DIGITS)) &&
	!print_digit(&bytes[i], third_digit_ptr, virt_row, virt_col, ALL_DIGITS_OFFSET_ROWS,
		     ALL_DIGITS_OFFSET_COLS + (2 * OLED_DIGIT_COLS) + OLED_POINT_COLS + (3 * SPACE_BETWEEN_DIGITS))) {
      bytes[i] = 0x00;
    }
  }
}

static void fill_oled_progress_gen_h(void) {
  FILE *fp = fopen("test/oled_progress_gen.h", "w+");

  fprintf(fp, "// DO NOT CHANGE THIS FILE! This file is generated.\n");
  fprintf(fp, "#ifndef TEST_OLED_PROGRESS_GEN_H_\n");
  fprintf(fp, "#define TEST_OLED_PROGRESS_GEN_H_\n");
  fprintf(fp, "\n");
  fprintf(fp, "#include <stdint.h>\n");
  fprintf(fp, "\n");
  fprintf(fp, "typedef struct {\n");
  fprintf(fp, "  uint8_t progress_in_pixels;\n");
  fprintf(fp, "  uint8_t disp_data[%d];\n", OLED_DISP_BYTES);
  fprintf(fp, "} oled_gen_progress_st;\n");
  fprintf(fp, "\n");
  fprintf(fp, "extern oled_gen_progress_st progress_tcs[%d];\n", OLED_COLS);
  fprintf(fp, "\n");
  fprintf(fp, "#endif // TEST_OLED_PROGRESS_GEN_H_\n");

  fflush(fp);

  fclose(fp);
}

static void print_time_data(FILE *fp, uint8_t *bytes) {
  fprintf(fp, "// +");
  for (size_t col = 0; col < OLED_COLS; col++) {
    fprintf(fp, "-");
  }
  fprintf(fp, "+\n");

  for (size_t row = 0; row < OLED_ROWS; row++) {
    for (size_t bit_row = 0; bit_row < CHAR_BIT; bit_row++) {
      fprintf(fp, "// |");
      for (size_t col = 0; col < OLED_COLS; col++) {
	uint8_t data = bytes[(row * OLED_COLS) + col];
	uint8_t bit = data & (1 << bit_row);
	fprintf(fp, "%s", (bit ? "#" : " "));
      }
      fprintf(fp, "|\n");
    }
  }

  fprintf(fp, "// +");
  for (size_t col = 0; col < OLED_COLS; col++) {
    fprintf(fp, "-");
  }
  fprintf(fp, "+\n");
}

static void print_array_data(FILE *fp, uint8_t *bytes) {
  for(size_t pix = 0; pix < OLED_DISP_BYTES; pix++) {
    fprintf(fp, "0x%X, ", bytes[pix]);
  }
}

static void fill_oled_progress_gen_c(void) {
  FILE *fp = fopen("test/oled_progress_gen.c", "w+");

  fprintf(fp, "// DO NOT CHANGE THIS FILE! This file is generated.\n");
  fprintf(fp, "#include \"oled_progress_gen.h\"\n");
  fprintf(fp, "\n");
  fprintf(fp, "oled_gen_progress_st progress_tcs[%d] = {\n", OLED_COLS);

  for (uint8_t i = 0; i < OLED_COLS; i++) {
    fprintf(fp, "  { .progress_in_pixels = %u,\n", i);

    uint8_t bytes[OLED_DISP_BYTES];
    fill_bytes(bytes, i, i);

    print_time_data(fp, bytes);

    fprintf(fp, "    .disp_data = {");
    print_array_data(fp, bytes);
    fprintf(fp, "},\n");

    fprintf(fp, "  },\n");
  }

  fprintf(fp, "};\n");

  fflush(fp);

  fclose(fp);
}

static void fill_oled_time_gen_h(void) {
  FILE *fp = fopen("test/oled_time_gen.h", "w+");

  fprintf(fp, "// DO NOT CHANGE THIS FILE! This file is generated.\n");
  fprintf(fp, "#ifndef TEST_OLED_TIME_GEN_H_\n");
  fprintf(fp, "#define TEST_OLED_TIME_GEN_H_\n");
  fprintf(fp, "\n");
  fprintf(fp, "#include <stdint.h>\n");
  fprintf(fp, "\n");
  fprintf(fp, "typedef struct {\n");
  fprintf(fp, "  uint16_t time;\n");
  fprintf(fp, "  uint8_t disp_data[%d];\n", OLED_DISP_BYTES);
  fprintf(fp, "} oled_gen_time_st;\n");
  fprintf(fp, "\n");
  fprintf(fp, "extern oled_gen_time_st time_tcs[%d];\n", MAX_TIME + 1);
  fprintf(fp, "\n");
  fprintf(fp, "#endif // TEST_OLED_TIME_GEN_H_\n");

  fflush(fp);

  fclose(fp);
}

static void fill_oled_time_gen_c(void) {
  FILE *fp = fopen("test/oled_time_gen.c", "w+");

  fprintf(fp, "// DO NOT CHANGE THIS FILE! This file is generated.\n");
  fprintf(fp, "#include \"oled_time_gen.h\"\n");
  fprintf(fp, "\n");
  fprintf(fp, "oled_gen_time_st time_tcs[%d] = {\n", MAX_TIME + 1);

  for (size_t i = 0; i <= MAX_TIME; i++) {
    fprintf(fp, "  { .time = %lu,\n", i);

    uint8_t bytes[OLED_DISP_BYTES];
    fill_bytes(bytes, i, 0);

    print_time_data(fp, bytes);

    fprintf(fp, "    .disp_data = {");
    print_array_data(fp, bytes);
    fprintf(fp, "},\n");

    fprintf(fp, "  },\n");
  }

  fprintf(fp, "};\n");

  fflush(fp);

  fclose(fp);
}

int main(void) {
  fill_oled_progress_gen_h();
  fill_oled_progress_gen_c();
  fill_oled_time_gen_h();
  fill_oled_time_gen_c();
  return 0;
}
