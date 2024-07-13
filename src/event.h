#pragma once

#include "connection.h"
#include <map>
#include <memory>
#include <xcb/xcb.h>

using EventType = int;
using Event = std::unique_ptr<xcb_generic_event_t, decltype(&free)>;

class EventHandler {
public:
  explicit EventHandler(Connection &connection);

  inline void DispatchEvent(EventType event_type, Event event);

  void WindowResize(int x_amount, int y_amount);
  void WindowMove(int x_amount, int y_amount);
  void WindowFocus(int x_amount, int y_amount);
  void WorkspaceFocus(uint32_t workspace_id);
  void WorkspaceWindowMove(uint32_t workspace_id);

private:
  void OnErrorEventHandler(EventType event_type, Event event);
  void OnConfigureRequestEventHandler(EventType event_type, Event event);
  void OnMotionNotifyEventHandler(EventType event_type, Event event);
  void OnKeyPressEventHandler(EventType event_type, Event event);
  void OnEnterNotifyEventHandler(EventType event_type, Event event);
  void OnLeaveNotifyEventHandler(EventType event_type, Event event);
  void OnFocusInEventHandler(EventType event_type, Event event);
  void OnFocusOutEventHandler(EventType event_type, Event event);
  void OnCreateNotifyEventHandler(EventType event_type, Event event);
  void OnDestroyNotifyEventHandler(EventType event_type, Event event);
  void OnMapRequestEventHandler(EventType event_type, Event event);
  void OnUnmapNotifyEventHandler(EventType event_type, Event event);
  void OnPropertyNotifyEventHandler(EventType event_type, Event event);
  void OnClientMessageEventHandler(EventType event_type, Event event);

private:
  Connection &connection_;
  std::size_t current_screen_;
  // std::map<std::size_t, std::vector<std::size_t>> workspaces_;
};

template <typename To> inline auto CastEventType(Event event) {
  return std::unique_ptr<To, decltype(event.get_deleter())>{
      reinterpret_cast<To *>(event.release()), event.get_deleter()};
}

inline void EventHandler::DispatchEvent(EventType event_type, Event event) {
  switch (event_type) {
  case 0:
    OnErrorEventHandler(event_type, std::move(event));
    break;
  case XCB_CONFIGURE_REQUEST:
    OnConfigureRequestEventHandler(event_type, std::move(event));
    break;
  case XCB_MOTION_NOTIFY:
    OnMotionNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_KEY_PRESS:
    OnKeyPressEventHandler(event_type, std::move(event));
    break;
  case XCB_ENTER_NOTIFY:
    OnEnterNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_LEAVE_NOTIFY:
    OnLeaveNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_FOCUS_IN:
    OnFocusInEventHandler(event_type, std::move(event));
    break;
  case XCB_FOCUS_OUT:
    OnFocusOutEventHandler(event_type, std::move(event));
    break;
  case XCB_CREATE_NOTIFY:
    OnCreateNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_DESTROY_NOTIFY:
    OnDestroyNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_MAP_REQUEST:
    OnMapRequestEventHandler(event_type, std::move(event));
    break;
  case XCB_UNMAP_NOTIFY:
    OnUnmapNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_PROPERTY_NOTIFY:
    OnPropertyNotifyEventHandler(event_type, std::move(event));
    break;
  case XCB_CLIENT_MESSAGE:
    OnClientMessageEventHandler(event_type, std::move(event));
    break;
  default:
    break;
  }
}
