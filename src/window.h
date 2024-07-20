#ifndef WINDOW_H
#define WINDOW_H

#include "window_manager.h"
#include "rect.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <array>

inline void WindowLower(xcb_window_t window) {
  auto      &wm         = WindowManager::Instance();
  auto       connection = wm.Connection();
  std::array values{XCB_STACK_MODE_BELOW};
  xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_STACK_MODE, values.data());
}

void WindowSetVisibility(xcb_window_t window, bool visible);
void WindowMoveResize(xcb_window_t window, const xcb_rectangle_t &rect);

#endif
