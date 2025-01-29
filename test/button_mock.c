#include "button.h"

#include "framework.h"
#include "mock.h"

button_e button_is_pushed(void) {
  type_st ret;

  mock_record(NULL, 0, &ret);

  if (ret.type == TYPE_BUTTON_E) {
    return *((button_e*)ret.value);
  }

  log_error("Invalid return type");
  return 0;
}
