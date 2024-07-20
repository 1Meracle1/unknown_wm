#ifndef MONITOR_H
#define MONITOR_H

#include "types.h"
#include <cstdint>
#include <iterator>
#include <limits>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

struct Monitor {
  std::string name;
  bool        primary;
  Vector2D    position;
  Vector2D    size;
  // Vector2D    top_left;
  // Vector2D    bottom_right;
};

inline int32_t g_randr_event_base{0};

auto SetupMonitors(xcb_connection_t *connection, xcb_window_t root) -> bool;
auto GetMonitorFromPos(xcb_connection_t *connection, Vector2D &&pos) -> const Monitor &;

#endif
