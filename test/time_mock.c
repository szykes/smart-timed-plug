#include "time.h"

#include "mock.h"

uint16_t time_get_for_display(void) {
  MOCK_RECORD_RET(TYPE_UINT16_T, uint16_t);
}
