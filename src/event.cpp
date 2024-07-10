#include "event.h"
#include <xcb/xcb_event.h>

void OnErrorEventHandler(Connection &connection, EventType event_type,
                         Event event) {
  auto error_label = std::string{xcb_event_get_error_label(event_type)};
  if (error_label != "Success") {
    ERRORF("Error event occurred: {}", error_label);
  }
}

void OnConfigureEventHandler(Connection &connection, EventType event_type,
                             Event event) {
  INFO("OnConfigureEventHandler");
}

void OnMotionNotifyEventHandler(Connection &connection, EventType event_type,
                                Event event) {}

void OnKeyPressEventHandler(Connection &connection, EventType event_type,
                            Event event) {}

void OnEnterNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event) {}

void OnLeaveNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event) {}

void OnFocusInEventHandler(Connection &connection, EventType event_type,
                           Event event) {}

void OnFocusOutEventHandler(Connection &connection, EventType event_type,
                            Event event) {}

void OnCreateNotifyEventHandler(Connection &connection, EventType event_type,
                                Event event) {}

void OnDestroyNotifyEventHandler(Connection &connection, EventType event_type,
                                 Event event) {}

void OnMapRequestEventHandler(Connection &connection, EventType event_type,
                              Event event) {}

void OnUnmapNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event) {}

void OnPropertyNotifyEventHandler(Connection &connection, EventType event_type,
                                  Event event) {}

void OnClientMessageEventHandler(Connection &connection, EventType event_type,
                                 Event event) {}
