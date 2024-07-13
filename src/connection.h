#pragma once

#include "logging.h"
#include <concepts>
#include <type_traits>
#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

template <typename T>
concept ConvertibleToU32 =
    std::convertible_to<std::remove_cvref_t<T>, uint32_t>;

class Connection {
public:
  static std::unique_ptr<Connection> Init();
  // Connection(const Connection &) = delete;
  // Connection &operator=(const Connection &) noexcept = delete;
  ~Connection();

  [[nodiscard]] inline const std::vector<xcb_screen_t *> &GetScreens() const;

  [[nodiscard]] inline xcb_connection_t *Get() const;

  void SubscribeWMEvents() const;

  inline auto ChangeWindowAttributesChecked(xcb_window_t window, uint32_t flags,
                                            uint32_t values) const;

  template <ConvertibleToU32 T, ConvertibleToU32... Ts>
  inline auto ConfigureWindow(xcb_window_t window, uint16_t values_mask,
                              T &&value, Ts &&...values) const;
  template <ConvertibleToU32... Ts>
  inline auto ConfigureWindow(xcb_window_t window, uint16_t values_mask,
                              Ts &&...values) const;
  template <typename... Ts>
  inline std::enable_if<(std::is_same<Ts, uint32_t>::value && ...), bool>::type
  ConfigureWindow(xcb_window_t window, uint16_t values_mask,
                  Ts &&...values) const;

  inline auto ConfigureWindow(xcb_window_t window, uint16_t values_mask,
                              const uint32_t *values_list) const;

  // @return pairs of window and corresponding screen id
  [[nodiscard]] std::vector<std::pair<xcb_window_t, std::size_t>>
  ListMappedWindows() const;

  inline bool SetInputFocus(uint8_t revert_to, xcb_window_t window) const;

  [[nodiscard]] std::optional<xcb_window_t> FocusedWindow() const;

  void RegisterKeybindings() const;
  void GrabKey(uint16_t modifier, xcb_keycode_t key) const;
  [[nodiscard]] std::vector<xcb_keycode_t>
  GetKeyCodes(xcb_keysym_t symbol) const;

  inline void Flush() const;
  [[nodiscard]] inline auto WaitForEvent() const;

  [[nodiscard]] inline auto GetWindowAttributes(xcb_window_t window) const;

private:
  explicit Connection(xcb_connection_t *connection,
                      std::vector<xcb_screen_t *> &&screens);

private:
  xcb_connection_t *connection_;
  std::vector<xcb_screen_t *> screens_;
  xcb_key_symbols_t *key_symbols_;
  xcb_errors_context_t *errors_context_{nullptr};
};

[[nodiscard]] inline const std::vector<xcb_screen_t *> &
Connection::GetScreens() const {
  return screens_;
}

inline xcb_connection_t *Connection::Get() const { return connection_; }

inline auto Connection::ChangeWindowAttributesChecked(xcb_window_t window,
                                                      uint32_t flags,
                                                      uint32_t values) const {
  bool is_ok = true;
  auto cookie =
      xcb_change_window_attributes_checked(connection_, window, flags, &values);
  auto error = xcb_request_check(connection_, cookie);
  if (error) {
    ERROR("Failed to change window attributes, error "
          "code: {}",
          xcb_errors_get_name_for_error(errors_context_, error->error_code,
                                        nullptr));
    free(error);
    is_ok = false;
  }
  return is_ok;
}

template <ConvertibleToU32 T, ConvertibleToU32... Ts>
inline auto Connection::ConfigureWindow(xcb_window_t window,
                                        uint16_t values_mask, T &&value,
                                        Ts &&...values) const {
  const uint32_t values_list[] = {
      static_cast<uint32_t>(std::forward<T>(value)),
      static_cast<uint32_t>(std::forward<Ts>(values))...};
  return ConfigureWindow(window, values_mask, values_list);
}

template <ConvertibleToU32... Ts>
inline auto Connection::ConfigureWindow(xcb_window_t window,
                                        uint16_t values_mask,
                                        Ts &&...values) const {
  return ConfigureWindow(window, values_mask,
                         static_cast<uint32_t>(std::forward<Ts>(values))...);
}

template <typename... Ts>
inline std::enable_if<(std::is_same<Ts, uint32_t>::value && ...), bool>::type
Connection::ConfigureWindow(xcb_window_t window, uint16_t values_mask,
                            Ts &&...values) const {
  const uint32_t values_list[] = {std::forward<Ts>(values)...};
  return ConfigureWindow(window, values_mask, values_list);
}

inline auto Connection::ConfigureWindow(xcb_window_t window,
                                        uint16_t values_mask,
                                        const uint32_t *values_list) const {
  auto cookie =
      xcb_configure_window(connection_, window, values_mask, values_list);
  auto error = xcb_request_check(connection_, cookie);
  if (error) {
    ERROR("Failed to configure window: {}",
          xcb_errors_get_name_for_error(errors_context_, error->error_code,
                                        nullptr));
    free(error);
    return false;
  }
  return true;
}

inline bool Connection::SetInputFocus(const uint8_t revert_to,
                                      const xcb_window_t window) const {
  auto cookie = xcb_set_input_focus_checked(connection_, revert_to, window,
                                            XCB_CURRENT_TIME);
  auto error = xcb_request_check(connection_, cookie);
  if (error) {
    ERROR("Failed to set focus window: {}",
          xcb_errors_get_name_for_error(errors_context_, error->error_code,
                                        nullptr));
    free(error);
    return false;
  }
  return true;
}

inline void Connection::Flush() const { xcb_flush(connection_); }

[[nodiscard]] inline auto Connection::WaitForEvent() const {
  return std::unique_ptr<xcb_generic_event_t, decltype(&free)>{
      xcb_wait_for_event(connection_), &free};
}

[[nodiscard]] inline auto
Connection::GetWindowAttributes(xcb_window_t window) const {
  std::unique_ptr<xcb_get_window_attributes_reply_t, decltype(&free)> attrs{
      nullptr, free};
  auto cookie = xcb_get_window_attributes(connection_, window);
  xcb_generic_error_t *error;
  auto reply = xcb_get_window_attributes_reply(connection_, cookie, &error);
  if (error) {
    ERROR("Failed to get window attributes: {}",
          xcb_errors_get_name_for_error(errors_context_, error->error_code,
                                        nullptr));
    return std::unique_ptr<xcb_get_window_attributes_reply_t, decltype(&free)>{
        nullptr, free};
  }
  return std::unique_ptr<xcb_get_window_attributes_reply_t, decltype(&free)>{
      reply, free};
}
