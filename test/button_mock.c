#include "button.h"

#include "mock.h"

button_event_e button_is_pushed(void) {
  MOCK_RECORD_RET(button_event_e);
}
