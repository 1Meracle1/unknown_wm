#pragma once

#include "connection.h"
#include <map>
#include <memory>
#include <xcb/xcb.h>

using EventType = int;
using Event = std::unique_ptr<xcb_generic_event_t, decltype(&free)>;
using EventHandler = std::function<void(Connection &, EventType, Event)>;

void OnErrorEventHandler(Connection &connection, EventType event_type,
                         Event event);
void OnConfigureEventHandler(Connection &connection, EventType event_type,
                             Event event);
void OnMotionNotifyEventHandler(Connection &connection, EventType event_type,
                                Event event);
void OnKeyPressEventHandler(Connection &connection, EventType event_type,
                            Event event);
void OnEnterNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event);
void OnLeaveNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event);
void OnFocusInEventHandler(Connection &connection, EventType event_type,
                           Event event);
void OnFocusOutEventHandler(Connection &connection, EventType event_type,
                            Event event);
void OnCreateNotifyEventHandler(Connection &connection, EventType event_type,
                                Event event);
void OnDestroyNotifyEventHandler(Connection &connection, EventType event_type,
                                 Event event);
void OnMapRequestEventHandler(Connection &connection, EventType event_type,
                              Event event);
void OnUnmapNotifyEventHandler(Connection &connection, EventType event_type,
                               Event event);
void OnPropertyNotifyEventHandler(Connection &connection, EventType event_type,
                                  Event event);
void OnClientMessageEventHandler(Connection &connection, EventType event_type,
                                 Event event);

static const std::map<EventType, EventHandler> event_map{
    {0, OnErrorEventHandler},
    {XCB_CONFIGURE_REQUEST, OnConfigureEventHandler},
    {XCB_MOTION_NOTIFY, OnMotionNotifyEventHandler},
    {XCB_KEY_PRESS, OnKeyPressEventHandler},
    {XCB_ENTER_NOTIFY, OnEnterNotifyEventHandler},
    {XCB_LEAVE_NOTIFY, OnLeaveNotifyEventHandler},
    {XCB_FOCUS_IN, OnFocusInEventHandler},
    {XCB_FOCUS_OUT, OnFocusOutEventHandler},
    {XCB_CREATE_NOTIFY, OnCreateNotifyEventHandler},
    {XCB_DESTROY_NOTIFY, OnDestroyNotifyEventHandler},
    {XCB_MAP_REQUEST, OnMapRequestEventHandler},
    {XCB_UNMAP_NOTIFY, OnUnmapNotifyEventHandler},
    {XCB_PROPERTY_NOTIFY, OnPropertyNotifyEventHandler},
    {XCB_CLIENT_MESSAGE, OnClientMessageEventHandler},
};

inline void DispatchEvent(Connection &connection, EventType event_type,
                          Event event) {
  if (auto it = event_map.find(event_type); it != event_map.end()) {
    it->second(connection, event_type, std::move(event));
  }
}
