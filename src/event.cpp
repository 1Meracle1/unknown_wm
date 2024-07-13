#include "event.h"
#include "config.h"
#include <cstdint>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

EventHandler::EventHandler(Connection &connection)
    : connection_{connection}, current_screen_{0} {}

void EventHandler::WindowResize(int x_amount, int y_amount) {}

void EventHandler::WindowMove(int x_amount, int y_amount) {}

void EventHandler::WindowFocus(int x_amount, int y_amount) {}

void EventHandler::WorkspaceFocus(uint32_t workspace_id) {}

void EventHandler::WorkspaceWindowMove(uint32_t workspace_id) {}

void EventHandler::OnErrorEventHandler(EventType event_type, Event event) {
  auto error_label = std::string{xcb_event_get_error_label(event_type)};
  if (error_label != "Success") {
    ERROR("Error event occurred: {}", error_label);
  }
}

void EventHandler::OnConfigureRequestEventHandler(EventType event_type,
                                                  Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_configure_request_event_t>(std::move(event));
  auto mask = e->value_mask;
  std::size_t i = 0;
  std::array<uint32_t, 7> values{};
  if (mask & XCB_CONFIG_WINDOW_X)
    values[i++] = static_cast<uint32_t>(e->x);
  if (mask & XCB_CONFIG_WINDOW_Y)
    values[i++] = static_cast<uint32_t>(e->y);
  if (mask & XCB_CONFIG_WINDOW_WIDTH)
    values[i++] = static_cast<uint32_t>(e->width);
  if (mask & XCB_CONFIG_WINDOW_HEIGHT)
    values[i++] = static_cast<uint32_t>(e->height);
  if (mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
    values[i++] = static_cast<uint32_t>(e->border_width);
  if (mask & XCB_CONFIG_WINDOW_SIBLING)
    values[i++] = e->sibling;
  if (mask & XCB_CONFIG_WINDOW_STACK_MODE)
    values[i++] = static_cast<uint32_t>(e->stack_mode);
  connection_.ConfigureWindow(e->window, mask, values.data());
}

void EventHandler::OnMotionNotifyEventHandler(EventType event_type,
                                              Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnKeyPressEventHandler(EventType event_type, Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnEnterNotifyEventHandler(EventType event_type,
                                             Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_enter_notify_event_t>(std::move(event));
  connection_.SetInputFocus(XCB_INPUT_FOCUS_PARENT, e->event);
}

void EventHandler::OnLeaveNotifyEventHandler(EventType event_type,
                                             Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnFocusInEventHandler(EventType event_type, Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_focus_in_event_t>(std::move(event));
  if (e->mode == XCB_NOTIFY_MODE_GRAB || e->mode == XCB_NOTIFY_MODE_UNGRAB ||
      e->detail == XCB_NOTIFY_DETAIL_POINTER ||
      e->detail == XCB_NOTIFY_DETAIL_POINTER_ROOT ||
      e->detail == XCB_NOTIFY_DETAIL_NONE) {
    return;
  }
  connection_.ChangeWindowAttributesChecked(e->event, XCB_CW_BORDER_PIXEL,
                                            BORDER_COLOR_ACTIVE);
}

void EventHandler::OnFocusOutEventHandler(EventType event_type, Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_focus_out_event_t>(std::move(event));
  if (e->mode == XCB_NOTIFY_MODE_GRAB || e->mode == XCB_NOTIFY_MODE_UNGRAB) {
    return;
  }
  connection_.ChangeWindowAttributesChecked(e->event, XCB_CW_BORDER_PIXEL,
                                            BORDER_COLOR_INACTIVE);
}

void EventHandler::OnCreateNotifyEventHandler(EventType event_type,
                                              Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_create_notify_event_t>(std::move(event));
  auto attrs = connection_.GetWindowAttributes(e->window);
  if (attrs->override_redirect) {
    return;
  }
  connection_.ChangeWindowAttributesChecked(
      e->window, XCB_CW_EVENT_MASK,
      XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
          XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);
  connection_.ConfigureWindow(e->window, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                              BORDER_WIDTH);
}

void EventHandler::OnDestroyNotifyEventHandler(EventType event_type,
                                               Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnMapRequestEventHandler(EventType event_type, Event event) {
  INFO("{}", xcb_event_get_label(event_type));
  auto e = CastEventType<xcb_map_request_event_t>(std::move(event));
  auto attrs = connection_.GetWindowAttributes(e->window);
  if (attrs && attrs->map_state == XCB_MAP_STATE_VIEWABLE) {
    if (auto focused_win = connection_.FocusedWindow(); focused_win) {
      connection_.ConfigureWindow(focused_win.value(),
                                  XCB_CONFIG_WINDOW_STACK_MODE,
                                  XCB_STACK_MODE_BELOW);
    }
    connection_.ConfigureWindow(e->window, XCB_CONFIG_WINDOW_STACK_MODE,
                                XCB_STACK_MODE_ABOVE);
    connection_.SetInputFocus(XCB_INPUT_FOCUS_PARENT, e->window);
  } else {
    connection_.ConfigureWindow(e->window, XCB_CONFIG_WINDOW_STACK_MODE,
                                XCB_STACK_MODE_BELOW);
  }
}

void EventHandler::OnUnmapNotifyEventHandler(EventType event_type,
                                             Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnPropertyNotifyEventHandler(EventType event_type,
                                                Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}

void EventHandler::OnClientMessageEventHandler(EventType event_type,
                                               Event event) {
  INFO("{}", xcb_event_get_label(event_type));
}
