#include "time.h"

#include "framework.h"
#include "mock.h"

uint16_t time_get_for_display(void) {
  type_st ret;

  mock_record(NULL, 0, &ret);

  if (ret.type == TYPE_UINT16_T) {
    return *((uint16_t*)ret.value);
  }

  log_error("Invalid return type");
  return 0;
}
