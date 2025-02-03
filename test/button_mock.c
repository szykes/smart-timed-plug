#include "button.h"

#include "mock.h"

button_event_e button_is_pushed(void) {
  MOCK_RECORD_RET(TYPE_BUTTON_EVENT_E, button_event_e);
}
