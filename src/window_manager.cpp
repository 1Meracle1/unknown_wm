#include "window_manager.h"
#include "ewmh.h"
#include "keys.h"
#include "logging.h"
#include "monitor.h"
#include "type_utils.h"
#include "types.h"
#include "xcb_utils.h"

#include <X11/X.h>
#include <X11/Xutil.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xcb/randr.h>
#include <xcb/xcb_cursor.h>
#include <xcb/shape.h>

auto WindowManager::Init() -> WindowManager {
  int  screen_number;
  auto connection = xcb_connect(nullptr, &screen_number);
  if (connection == nullptr) {
    ERROR("Failed to connect to X11.");
    return WindowManager();
  }
  auto conn_error = xcb_connection_has_error(connection);
  if (conn_error > 0) {
    ERROR("XCB connection error: {}.", conn_error);
    xcb_disconnect(connection);
    return WindowManager();
  }
  xcb_screen_t *screen          = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
  auto          visual_type     = xcb_aux_find_visual_by_attrs(screen, -1, 32); // transparency
  auto          colormap        = xcb_aux_get_depth_of_visual(screen, visual_type->visual_id);
  auto          colormap_cookie = xcb_create_colormap(connection, XCB_COLORMAP_ALLOC_NONE, colormap,
                                                      screen->root, visual_type->visual_id);
  auto          error           = xcb_request_check(connection, colormap_cookie);
  if (error) {
    ERROR("Failed to create colormap: {}", error->error_code);
    return WindowManager();
  }

  auto ewmh = Ewmh::Init(connection, screen_number, screen);
  if (!ewmh.IsValid()) {
    ERROR("Failed to initialize ewmh.");
    return WindowManager();
  }

  if (!SetupMonitors(connection, screen->root)) {
    ERROR("Failed to setup monitors.");
    return WindowManager();
  }

  std::array events{XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE};
  xcb_change_window_attributes_checked(connection, screen->root, XCB_CW_EVENT_MASK, events.data());

  return WindowManager(connection, screen, std::move(ewmh));
}

WindowManager::WindowManager(xcb_connection_t *connection, xcb_screen_t *screen, Ewmh &&ewmh)
    : is_valid_{true}, connection_{connection}, screen_{screen}, ewmh_{std::move(ewmh)} {}

void WindowManager::Run() {
  for (;;) {
    if (xcb_connection_has_error(connection_)) {
      break;
    }
    xcb_flush(connection_);

    // receive event
    auto event = PtrWrap(xcb_wait_for_event(connection_));
    if (event) {
      auto event_type = XCB_EVENT_RESPONSE_TYPE(event);
      // auto event_code = event->response_type & ~0x80;

      switch (event_type) {
      case 0:
        OnErrorEventHandler(event_type, std::move(event));
        break;
      case XCB_CONFIGURE_REQUEST:
        OnConfigureRequestEventHandler(std::move(event));
        break;
      case XCB_MOTION_NOTIFY:
        OnMotionNotifyEventHandler(std::move(event));
        break;
      case XCB_KEY_PRESS:
        OnKeyPressEventHandler(std::move(event));
        break;
      case XCB_ENTER_NOTIFY:
        OnEnterNotifyEventHandler(std::move(event));
        break;
      case XCB_LEAVE_NOTIFY:
        OnLeaveNotifyEventHandler(std::move(event));
        break;
      case XCB_FOCUS_IN:
        OnFocusInEventHandler(std::move(event));
        break;
      case XCB_FOCUS_OUT:
        OnFocusOutEventHandler(std::move(event));
        break;
      case XCB_CREATE_NOTIFY:
        OnCreateNotifyEventHandler(std::move(event));
        break;
      case XCB_DESTROY_NOTIFY:
        OnDestroyNotifyEventHandler(std::move(event));
        break;
      case XCB_MAP_REQUEST:
        OnMapRequestEventHandler(std::move(event));
        break;
      case XCB_UNMAP_NOTIFY:
        OnUnmapNotifyEventHandler(std::move(event));
        break;
      case XCB_PROPERTY_NOTIFY:
        OnPropertyNotifyEventHandler(std::move(event));
        break;
      case XCB_CLIENT_MESSAGE:
        OnClientMessageEventHandler(std::move(event));
        break;
      default:
        break;
      }

      if (g_randr_event_base > 0 &&
          event_type - g_randr_event_base == XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
        OnRandrScreenChange(std::move(event));
      }
    }
  }
}

auto WindowManager::UpdateRootCursor() -> bool {
  if (xcb_cursor_context_new(connection_, screen_, cursor_context_) != 0) {
    ERROR("Failed to create cursor context.");
    return false;
  }
  return true;
  cursor_ = xcb_cursor_load_cursor(*cursor_context_, "left_ptr");
  xcb_change_window_attributes(connection_, screen_->root, XCB_CW_CURSOR, &cursor_);
}

void WindowManager::OnErrorEventHandler([[maybe_unused]] EventType event_type,
                                        [[maybe_unused]] Event     event) {}

void WindowManager::OnConfigureRequestEventHandler([[maybe_unused]] Event event) {
  // auto evt = reinterpret_cast<xcb_configure_request_event_t *>(event.get());
  INFO("Configure request");
}

void WindowManager::OnMotionNotifyEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnKeyPressEventHandler([[maybe_unused]] Event event) {
  auto evt  = reinterpret_cast<xcb_key_press_event_t *>(event.get());
  auto syms = std::unique_ptr<xcb_key_symbols_t, decltype(&xcb_key_symbols_free)>{
      xcb_key_symbols_alloc(connection_), &xcb_key_symbols_free};
  {
    auto enter_keys = xcb_key_symbols_get_keycode(syms.get(), Keys::Return);
    for (auto keycode = enter_keys; *keycode != XCB_NO_SYMBOL; ++keycode) {
      if ((evt->state & Keys::Alt) && evt->detail == *keycode) {
        INFO("Terminal to be called.");
        if (fork() == 0) {
          execl("/bin/sh", "/bin/sh", "-c", "xterm", nullptr);
          _exit(0);
        }
        return;
      }
    }
  }
  {
    auto keys = xcb_key_symbols_get_keycode(syms.get(), XK_h);
    for (auto keycode = keys; *keycode != XCB_NO_SYMBOL; ++keycode) {
      if ((evt->state & Keys::Alt) && evt->detail == *keycode) {
        INFO("Focus to switch to another window to the left.");
        if (windows_.size() > 1) {
          if (!last_focused_window_) {
            last_focused_window_ = windows_.back().id;
          }
          auto it = std::ranges::find_if(
              windows_, [last_focused_window = last_focused_window_.value()](const auto &win) {
                return win.id == last_focused_window;
              });
          if (it != std::ranges::end(windows_)) {
            auto        &focused_window   = *it;
            xcb_window_t next_window      = XCB_WINDOW_NONE;
            auto         left_closest_pos = Vector2D{10000, 10000};
            for (auto &window : windows_ | std::views::filter([&focused_window](const auto &win) {
                                  return win.id != focused_window.id &&
                                         win.position.x < focused_window.position.x;
                                })) {
              if (left_closest_pos.x > window.position.x &&
                  left_closest_pos.y > window.position.x) {
                left_closest_pos = window.position;
                next_window      = window.id;
              }
            }
            if (next_window != XCB_WINDOW_NONE) {
              ApplyFocusToWindow(next_window);
            }
          } else {
            ERROR("Failed to find focused window in the windows_ vector.");
          }
        }
        return;
      }
    }
  }
  {
    auto keys = xcb_key_symbols_get_keycode(syms.get(), XK_l);
    for (auto keycode = keys; *keycode != XCB_NO_SYMBOL; ++keycode) {
      if ((evt->state & Keys::Alt) && evt->detail == *keycode) {
        INFO("Focus to switch to another window to the right.");
        return;
      }
    }
  }
}

void WindowManager::OnEnterNotifyEventHandler([[maybe_unused]] Event event) {
  INFO("On Enter.");
  auto     evt      = reinterpret_cast<xcb_enter_notify_event_t *>(event.get());
  uint32_t values[] = {0x9299f7};
  xcb_configure_window(connection_, evt->event, XCB_CW_BORDER_PIXEL, values);
}

void WindowManager::OnLeaveNotifyEventHandler([[maybe_unused]] Event event) {
  INFO("On Leave.");
  auto     evt      = reinterpret_cast<xcb_leave_notify_event_t *>(event.get());
  uint32_t values[] = {0x0a0a0a};
  xcb_configure_window(connection_, evt->event, XCB_CW_BORDER_PIXEL, values);
}

void WindowManager::OnFocusInEventHandler([[maybe_unused]] Event event) {
  auto evt = reinterpret_cast<xcb_focus_in_event_t *>(event.get());
  INFO("Focus In.");
  ApplyFocusToWindow(evt->event);
}

void WindowManager::OnFocusOutEventHandler([[maybe_unused]] Event event) {
  auto evt = reinterpret_cast<xcb_focus_out_event_t *>(event.get());
  INFO("Focus Out.");
  UnapplyFocusToWindow(evt->event);
}

void WindowManager::OnCreateNotifyEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnDestroyNotifyEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnMapRequestEventHandler([[maybe_unused]] Event event) {
  auto evt = reinterpret_cast<xcb_map_request_event_t *>(event.get());
  INFO("Map Request");
  xcb_map_window(connection_, evt->window);
  if (IsWindowUnmapped(evt->window)) {
    MoveUnmappedWindowToMapped(evt->window);
  } else {
    if (!IsWindowMapped(evt->window) && IsWindowManagable(evt->window)) {
      auto &new_window = AddWindowToMapped(evt->window);
      RemapWindow(new_window);
    }
  }
}

void WindowManager::OnUnmapNotifyEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnPropertyNotifyEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnClientMessageEventHandler([[maybe_unused]] Event event) {}

void WindowManager::OnRandrScreenChange([[maybe_unused]] Event event) {
  SetupMonitors(connection_, screen_->root);
}

auto WindowManager::IsWindowManagable([[maybe_unused]] xcb_window_t window) -> bool {
  auto [attrs, is_ok] = XcbReplyWrapPtr(connection_, xcb_get_window_attributes,
                                        xcb_get_window_attributes_reply, window);
  if (!is_ok || !attrs) {
    WARN("Failed to fetch window attributes");
    return false;
  }

  auto [geometry, is_geometry_ok] =
      XcbReplyWrapPtr(connection_, xcb_get_geometry, xcb_get_geometry_reply, window);
  if (!is_geometry_ok || !geometry) {
    WARN("Failed to fetch window geometry");
    return false;
  }

  return !attrs->override_redirect;
}

void WindowManager::RemapWindow(WmWindow &new_window) {
  {
    uint32_t window_events[] = {XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
                                XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    xcb_change_window_attributes(connection_, new_window.id, XCB_CW_EVENT_MASK, window_events);
  }
  if (auto reply = ewmh_.Property("_NET_WM_NAME", new_window.id); reply) {
    auto len = xcb_get_property_value_length(reply.get());
    if (len > 0) {
      auto data       = reinterpret_cast<char *>(xcb_get_property_value(reply.get()));
      new_window.name = std::string(data, data + len);
    }
  } else {
    ERROR("Failed to fetch window name.");
  }

  const auto &monitor = GetMonitorFromPos(connection_, CursorPosition());
  INFO("Monitor pos - x:{},y:{}.", monitor.position.x, monitor.position.y);
  INFO("Monitor size - x:{},y:{}.", monitor.size.x, monitor.size.y);
  const auto monitor_center =
      Vector2D(monitor.position.x + monitor.size.x / 2, monitor.position.y + monitor.size.y / 2);
  INFO("Monitor center - x:{},y:{}.", monitor_center.x, monitor_center.y);
  auto [wm_class_r, is_ok] =
      XcbReplyWrapPtr(connection_, xcb_get_property, xcb_get_property_reply, false, new_window.id,
                      XCB_ATOM_WM_CLASS, XCB_GET_PROPERTY_TYPE_ANY, 0, 128);
  // auto [geometry, is_geometry_ok] =
  //     XcbReplyWrapPtr(connection_, xcb_get_geometry, xcb_get_geometry_reply, window.id);
  // if (is_geometry_ok && geometry) {
  //   window.default_position = Vector2D(geometry->x, geometry->y);
  //   window.default_size     = Vector2D(geometry->width, geometry->height);
  // } else {
  //   window.default_position = Vector2D(0, 0);
  //   window.default_size     = Vector2D(screen_->width_in_pixels / 2, screen_->height_in_pixels /
  //   2);
  // }
  // ewmh_.GrabICCCMSizeHints(window);

  [[maybe_unused]] auto adjust_horiz = [](WmWindow &window, auto value) {
    window.position.x += value;
    window.size.x -= value * 2;
  };
  [[maybe_unused]] auto adjust_left = [](WmWindow &window, auto value) {
    window.position.x += value;
    window.size.x -= value;
  };
  [[maybe_unused]] auto adjust_right = [](WmWindow &window, auto value) { window.size.x -= value; };
  [[maybe_unused]] auto adjust_vert  = [](WmWindow &window, auto value) {
    window.position.y += value;
    window.size.y -= value * 2;
  };
  [[maybe_unused]] auto is_visible = [](const WmWindow &window, const Monitor &monitor) {
    return ((window.position.x >= monitor.position.x &&
             window.position.x < monitor.position.x + monitor.size.x) ||
            (window.position.x <= monitor.position.x &&
             window.position.x + window.size.x > monitor.position.x)) &&
           ((window.position.y >= monitor.position.y &&
             window.position.y < monitor.position.y + monitor.size.y) ||
            (window.position.y <= monitor.position.y &&
             window.position.y + window.size.y > monitor.position.y));
  };

  constexpr int32_t horizontally_stacked_windows = 3;
  static_assert(horizontally_stacked_windows > 0);

  // move all windows except the last one up,
  // considering 3 stacked windows per monitor
  auto windows = std::ranges::reverse_view(windows_) |
                 std::views::filter([id = new_window.id](const auto &win) { return win.id != id; });
  int64_t count = std::ranges::distance(windows);
  INFO("Count: {}.", count);
  if (count > 0 && count % horizontally_stacked_windows == 0) {
    INFO("Moving windows up.");
    for (auto &window : windows) {
      window.position.y -= monitor.size.y;
    }
  }
  if (count == 0 || count % horizontally_stacked_windows == 0) {
    new_window.size     = Vector2D(monitor.size.x / horizontally_stacked_windows, monitor.size.y);
    new_window.position = Vector2D(monitor_center.x - new_window.size.x / 2,
                                   monitor_center.y - new_window.size.y / 2);
    adjust_horiz(new_window, border_ + gap_);
    adjust_vert(new_window, border_ + gap_);
  } else if (count % horizontally_stacked_windows > 0) {
    auto dirty_windows = std::views::reverse(windows_) |
                         std::views::take(count % horizontally_stacked_windows + 1) |
                         std::views::reverse;
    INFO("Count of dirty windows: {}.", std::ranges::distance(dirty_windows));
    const auto width_per_window = monitor.size.x / horizontally_stacked_windows;
    int32_t    shift_x_pos      = monitor.position.x;
    for (auto &window : dirty_windows) {
      window.position.x = shift_x_pos;
      window.position.y = 0;
      window.size.x     = width_per_window;
      window.size.y     = monitor.size.y;
      shift_x_pos += window.size.x;
    }
    int32_t adjust_from_left_size =
        shift_x_pos < monitor.size.x ? (monitor.size.x - shift_x_pos) / 2 : 0;
    bool is_first = true;
    for (auto &window : dirty_windows) {
      window.position.x += adjust_from_left_size;
      if (is_first) {
        is_first = false;
        adjust_horiz(window, gap_);
      } else {
        adjust_right(window, gap_);
      }
      adjust_horiz(window, border_);
      adjust_vert(window, gap_ + border_);
    }
  }
  for (const auto &window : windows_) {
    INFO("x:{},y:{},w:{},h:{}", window.position.x, window.position.y, window.size.x, window.size.y);
    xcb_configure_window(connection_, window.id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                         Vector2DToArray(window.position).data());
    xcb_configure_window(connection_, window.id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         Vector2DToArray(window.size).data());
  }
  for (auto &window : windows) {
    window.position.y -= monitor.size.y;
    window.is_mapped = is_visible(window, monitor);
    if (window.is_mapped) {
      xcb_unmap_window(connection_, window.id);
    } else {
      xcb_map_window(connection_, window.id);
    }
  }

  // ApplyShapeToWindow(new_window);
  ApplyFocusToWindow(new_window.id);
}

[[nodiscard]] Vector2D WindowManager::CursorPosition() const {
  auto [pointer, is_ok] =
      XcbReplyWrapPtr(connection_, xcb_query_pointer, xcb_query_pointer_reply, screen_->root);
  if (!is_ok) {
    WARN("Failed to retrieve cursor pointer.");
    return Vector2D(0, 0);
  }
  return Vector2D(pointer->root_x, pointer->root_y);
}

void WindowManager::ApplyShapeToWindow(WmWindow &window) {
  auto shape_query = xcb_get_extension_data(connection_, &xcb_shape_id);
  if (!shape_query || !shape_query->present)
    return;

  int16_t   rounding        = 10;
  auto      width           = static_cast<uint16_t>(window.size.x + 2 * border_);
  auto      height          = static_cast<uint16_t>(window.size.y + 2 * border_);
  auto      radius          = static_cast<int16_t>(rounding + border_);
  auto      diameter        = static_cast<uint16_t>(radius == 0 ? 0 : radius * 2 - 1);
  xcb_arc_t bounding_arcs[] = {
      {-1, -1, diameter, diameter, 0, 360 << 6},
      {-1, static_cast<int16_t>(height - diameter), diameter, diameter, 0, 360 << 6},
      {static_cast<int16_t>(width - diameter), -1, diameter, diameter, 0, 360 << 6},
      {static_cast<int16_t>(width - diameter), static_cast<int16_t>(height - diameter), diameter,
       diameter, 0, 360 << 6},
  };
  xcb_rectangle_t bounding_rects[] = {
      {radius, 0, static_cast<uint16_t>(width - diameter), height},
      {0, radius, width, static_cast<uint16_t>(height - diameter)},
  };
  auto      diameterc       = static_cast<uint16_t>(rounding == 0 ? 0 : 2 * rounding - 1);
  xcb_arc_t clipping_arcs[] = {
      {-1, -1, diameterc, diameterc, 0, 360 << 6},
      {-1, static_cast<int16_t>(window.size.y - diameterc), diameterc, diameterc, 0, 360 << 6},
      {static_cast<int16_t>(window.size.x - diameterc), -1, diameterc, diameterc, 0, 360 << 6},
      {static_cast<int16_t>(window.size.x - diameterc),
       static_cast<int16_t>(window.size.y - diameterc), diameterc, diameterc, 0, 360 << 6},
  };
  xcb_rectangle_t clipping_rects[] = {{rounding, 0,
                                       static_cast<uint16_t>(window.size.x - diameterc),
                                       static_cast<uint16_t>(window.size.y)},
                                      {0, rounding, static_cast<uint16_t>(window.size.x),
                                       static_cast<uint16_t>(window.size.y - diameterc)}};

  xcb_pixmap_t bounding_pixmap = xcb_generate_id(connection_);
  xcb_pixmap_t clip_pixmap     = xcb_generate_id(connection_);

  xcb_gcontext_t black = xcb_generate_id(connection_);
  xcb_gcontext_t white = xcb_generate_id(connection_);

  xcb_create_pixmap(connection_, 1, bounding_pixmap, window.id, width, height);
  xcb_create_pixmap(connection_, 1, clip_pixmap, window.id, static_cast<uint16_t>(window.size.x),
                    static_cast<uint16_t>(window.size.y));

  uint32_t values[] = {0, 0};
  xcb_create_gc(connection_, black, bounding_pixmap, XCB_GC_FOREGROUND, values);
  values[0] = 1;
  xcb_create_gc(connection_, white, bounding_pixmap, XCB_GC_FOREGROUND, values);

  xcb_rectangle_t bounding_rect = {0, 0, static_cast<uint16_t>(window.size.x + 2 * border_),
                                   static_cast<uint16_t>(window.size.y + 2 * border_)};
  xcb_poly_fill_rectangle(connection_, bounding_pixmap, black, 1, &bounding_rect);
  xcb_poly_fill_rectangle(connection_, bounding_pixmap, white, 2, bounding_rects);
  xcb_poly_fill_arc(connection_, bounding_pixmap, white, 4, bounding_arcs);

  xcb_rectangle_t clipping_rect = {0, 0, static_cast<uint16_t>(window.size.x),
                                   static_cast<uint16_t>(window.size.y)};
  xcb_poly_fill_rectangle(connection_, clip_pixmap, black, 1, &clipping_rect);
  xcb_poly_fill_rectangle(connection_, clip_pixmap, white, 2, clipping_rects);
  xcb_poly_fill_arc(connection_, clip_pixmap, white, 4, clipping_arcs);

  xcb_shape_mask(connection_, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, window.id,
                 static_cast<int16_t>(-border_), static_cast<int16_t>(-border_), bounding_pixmap);
  xcb_shape_mask(connection_, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, window.id, 0, 0, clip_pixmap);

  xcb_free_pixmap(connection_, bounding_pixmap);
  xcb_free_pixmap(connection_, clip_pixmap);
}

void WindowManager::ApplyFocusToWindow(xcb_window_t window) {
  if (window != screen_->root) {
    auto it =
        std::ranges::find_if(windows_, [window](const auto &win) { return win.id == window; });
    if (it != std::ranges::end(windows_)) {
      if (last_focused_window_ && last_focused_window_.value() != window) {
        if (!it->is_mapped) {
          xcb_map_window(connection_, it->id);
          it->is_mapped = true;
        }
        INFO("Applying focus to a window.");
        xcb_ungrab_pointer(connection_, XCB_CURRENT_TIME);

        xcb_set_input_focus(connection_, XCB_INPUT_FOCUS_PARENT, XCB_NONE, XCB_CURRENT_TIME);
        xcb_set_input_focus(connection_, XCB_INPUT_FOCUS_PARENT, window, XCB_CURRENT_TIME);
        xcb_set_input_focus(connection_, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);

        uint32_t values[] = {0x9299f7};
        xcb_configure_window(connection_, window, XCB_CW_BORDER_PIXEL, values);

        ewmh_.UpdateCurrentWindow(window);
        ewmh_.UpdateWindow(window, true);
        ewmh_.UpdateWindow(last_focused_window_.value(), false);
        last_focused_window_ = std::optional<xcb_window_t>{window};
      }
    }
  }
}

void WindowManager::UnapplyFocusToWindow(xcb_window_t window) {
  if (window != screen_->root) {
    if (last_focused_window_ && last_focused_window_.value() == window) {
      INFO("Unapplying focus to a window.");
      xcb_ungrab_pointer(connection_, XCB_CURRENT_TIME);

      xcb_set_input_focus(connection_, XCB_INPUT_FOCUS_PARENT, XCB_NONE, XCB_CURRENT_TIME);

      uint32_t values[] = {0x0a0a0a};
      xcb_configure_window(connection_, window, XCB_CW_BORDER_PIXEL, values);

      ewmh_.UpdateWindow(last_focused_window_.value(), false);
      last_focused_window_ = std::nullopt;
    }
  }
}
