#include "connection.h"
#include "config.h"
#include "keys.h"
#include <memory>
#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_keysyms.h>

std::unique_ptr<Connection> Connection::Init() {
  int screen_number;
  auto conn = xcb_connect(nullptr, &screen_number);
  if (conn == nullptr) {
    return nullptr;
  }
  auto error = xcb_connection_has_error(conn);
  if (error > 0) {
    ERROR("XCB connection error: {}", error);
    xcb_disconnect(conn);
    return nullptr;
  }

  std::vector<xcb_screen_t *> screens;
  INFO("Screens expected: {}", screen_number);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
  for (; iter.rem; --screen_number, xcb_screen_next(&iter)) {
    screens.emplace_back(iter.data);
  }
  INFO("Screens found: {}", screens.size());

  return std::unique_ptr<Connection>(new Connection(conn, std::move(screens)));
}

Connection::Connection(xcb_connection_t *connection,
                       std::vector<xcb_screen_t *> &&screens)
    : connection_{connection}, screens_{std::move(screens)},
      key_symbols_{xcb_key_symbols_alloc(connection_)} {
  if (xcb_errors_context_new(connection_, &errors_context_)) {
    ERROR("Failed to initialize errors context");
  }
}

Connection::~Connection() {
  xcb_errors_context_free(errors_context_);
  xcb_key_symbols_free(key_symbols_);
  xcb_disconnect(connection_);
}

void Connection::SubscribeWMEvents() const {
  for (const auto screen : screens_) {
    ChangeWindowAttributesChecked(screen->root, XCB_CW_EVENT_MASK,
                                  XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                                      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                                      XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT);
  }
}

[[nodiscard]] std::vector<std::pair<xcb_window_t, std::size_t>>
Connection::ListMappedWindows() const {
  std::vector<std::pair<xcb_query_tree_cookie_t, std::size_t>>
      query_tree_cookies;
  query_tree_cookies.reserve(screens_.size());
  for (std::size_t i = 0; i < screens_.size(); ++i) {
    query_tree_cookies.emplace_back(
        xcb_query_tree(connection_, screens_[i]->root), i);
  }

  std::vector<std::pair<xcb_window_t, std::size_t>> windows;
  for (auto &[request_cookie, screen_id] : query_tree_cookies) {
    // errors never generated by this type of request
    xcb_generic_error_t *error;
    auto reply = xcb_query_tree_reply(connection_, request_cookie, &error);
    if (reply) {
      auto tree_children = xcb_query_tree_children(reply);
      const auto tree_children_count = xcb_query_tree_children_length(reply);
      auto parent_windows = std::vector<xcb_window_t>{
          tree_children, tree_children + tree_children_count};
      std::reverse(parent_windows.begin(), parent_windows.end());

      // exclude windows that are not mapped or have explicitly requested that
      std::vector<xcb_get_window_attributes_cookie_t> win_attrs_cookie;
      win_attrs_cookie.reserve(parent_windows.size());
      for (auto &win : parent_windows) {
        win_attrs_cookie.emplace_back(
            xcb_get_window_attributes(connection_, win));
      }
      for (std::size_t i = 0; i < win_attrs_cookie.size(); ++i) {
        auto win_attr_reply = xcb_get_window_attributes_reply(
            connection_, win_attrs_cookie[i], &error);
        if (reply && win_attr_reply->map_state == XCB_MAP_STATE_VIEWABLE &&
            !win_attr_reply->override_redirect) {
          windows.emplace_back(parent_windows[i], screen_id);
          free(win_attr_reply);
        }
      }
      free(reply);
    }
  }
  return windows;
}

[[nodiscard]] std::optional<xcb_window_t> Connection::FocusedWindow() const {
  std::optional<xcb_window_t> res = std::nullopt;
  xcb_generic_error_t *error;
  auto focus_reply = xcb_get_input_focus_reply(
      connection_, xcb_get_input_focus(connection_), &error);
  if (error) {
    ERROR("Failed to get input focus reply: {}",
          xcb_errors_get_name_for_error(errors_context_, error->error_code,
                                        nullptr));
  } else if (focus_reply->focus != XCB_NONE) {
    bool is_root = false;
    for (auto screen : screens_) {
      if (screen->root == focus_reply->focus) {
        is_root = true;
        break;
      }
    }
    if (!is_root) {
      res = focus_reply->focus;
    }
  }
  free(focus_reply);
  return res;
}

void Connection::RegisterKeybindings() const {
  // in case those are toggled we can still handle our other keybindings not
  // being affected by additional keys added to the combination
  constexpr std::array toggled_modifiers{0u,
                                         Keys::CapsLock,
                                         Keys::NumLock,
                                         Keys::ScrollLock,
                                         Keys::CapsLock | Keys::NumLock,
                                         Keys::CapsLock | Keys::ScrollLock,
                                         Keys::NumLock | Keys::ScrollLock,
                                         Keys::CapsLock | Keys::NumLock |
                                             Keys::ScrollLock};

  for (auto screen : screens_) {
    xcb_ungrab_key(connection_, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

    for (const auto &[key, action] : KEYBINDINGS) {
      for (const auto keycode : GetKeyCodes(key.key_symbol)) {
        for (const auto toggled_modifier : toggled_modifiers) {
          GrabKey(key.modifier | toggled_modifier, key.key_symbol);
        }
      }
    }
  }
}

void Connection::GrabKey(uint16_t modifier, xcb_keycode_t key) const {
  for (auto screen : screens_) {
    auto cookie =
        xcb_grab_key_checked(connection_, true, screen->root, modifier, key,
                             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    auto error = xcb_request_check(connection_, cookie);
    if (error) {
      ERROR("Failed to grab key: {}, error: {}", key, error->error_code);
      free(error);
    }
  }
}

[[nodiscard]] std::vector<xcb_keycode_t>
Connection::GetKeyCodes(xcb_keysym_t symbol) const {
  auto keycodes = std::unique_ptr<xcb_keycode_t[], decltype(&free)>{
      xcb_key_symbols_get_keycode(key_symbols_, symbol), free};
  std::size_t count = 0;
  for (; keycodes[count] != XCB_NO_SYMBOL; ++count)
    ;
  return std::vector<xcb_keycode_t>{keycodes.get(), keycodes.get() + count};
}
