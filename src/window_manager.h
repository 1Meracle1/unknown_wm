#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "ewmh.h"
#include "types.h"
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xproto.h>
#include <memory>

using EventType = int32_t;
using Event     = std::unique_ptr<xcb_generic_event_t, decltype(&free)>;

class WindowManager {
public:
  static auto                      Init() -> WindowManager;
  [[nodiscard]] static inline auto Instance() -> WindowManager & {
    if (!instance_.is_valid_) {
      instance_ = Init();
    }
    return instance_;
  }

  WindowManager(WindowManager &&)                 = default;
  WindowManager(const WindowManager &)            = delete;
  WindowManager &operator=(const WindowManager &) = delete;
  WindowManager &operator=(WindowManager &&)      = default;

  [[nodiscard]] inline auto IsValid() const -> bool { return is_valid_; }
  [[nodiscard]] inline auto Connection() const -> xcb_connection_t * { return connection_; }
  [[nodiscard]] inline auto Screen() const -> xcb_screen_t * { return screen_; }

  void Run();

private:
  explicit WindowManager() = default;
  explicit WindowManager(xcb_connection_t *connection, xcb_screen_t *screen, Ewmh &&ewmh);

  auto UpdateRootCursor() -> bool;

  void OnErrorEventHandler(EventType event_type, Event event);
  void OnConfigureRequestEventHandler(Event event);
  void OnMotionNotifyEventHandler(Event event);
  void OnKeyPressEventHandler(Event event);
  void OnEnterNotifyEventHandler(Event event);
  void OnLeaveNotifyEventHandler(Event event);
  void OnFocusInEventHandler(Event event);
  void OnFocusOutEventHandler(Event event);
  void OnCreateNotifyEventHandler(Event event);
  void OnDestroyNotifyEventHandler(Event event);
  void OnMapRequestEventHandler(Event event);
  void OnUnmapNotifyEventHandler(Event event);
  void OnPropertyNotifyEventHandler(Event event);
  void OnClientMessageEventHandler(Event event);
  void OnRandrScreenChange(Event event);

  inline auto IsWindowManagable(xcb_window_t window) -> bool;
  inline auto IsWindowUnmapped(xcb_window_t window) -> bool;
  inline auto IsWindowMapped(xcb_window_t window) -> bool;
  inline void MoveUnmappedWindowToMapped(xcb_window_t window);
  inline auto AddWindowToMapped(xcb_window_t window) -> WmWindow &;

  void                   RemapWindow(WmWindow &window);
  [[nodiscard]] Vector2D CursorPosition() const;
  void                   ApplyShapeToWindow(WmWindow &window);
  void                   FocusWindow(xcb_window_t window);
  // void                   RecalculateSizePosAll();
  // void                   RecalculateSizePosAroundFocused();
  // void                   RecalculateSizePosFocused();
  // void                   AdjustEffectiveSize(WmWindow &window) const;

private:
  static WindowManager instance_;
  bool                 is_valid_{false};
  xcb_connection_t    *connection_{nullptr};
  xcb_screen_t        *screen_{nullptr};
  Ewmh                 ewmh_{};

  xcb_cursor_context_t **cursor_context_{nullptr};
  xcb_cursor_t           cursor_{XCB_CURSOR_NONE};

  uint16_t border_ = 2;
  uint16_t gap_    = 10;

  std::vector<WmWindow>       windows_;
  std::optional<xcb_window_t> last_focused_window_{std::nullopt};
};

inline auto WindowManager::IsWindowUnmapped(xcb_window_t window) -> bool {
  auto it = std::ranges::find_if(windows_, [window](const auto &win) { return win.id == window; });
  if (it != std::ranges::end(windows_)) {
    return !it->is_mapped;
  }
  return false;
}

inline auto WindowManager::IsWindowMapped(xcb_window_t window) -> bool {
  auto it = std::ranges::find_if(windows_, [window](const auto &win) { return win.id == window; });
  if (it != std::ranges::end(windows_)) {
    return it->is_mapped;
  }
  return false;
}

inline void WindowManager::MoveUnmappedWindowToMapped(xcb_window_t window) {
  auto it = std::ranges::find_if(windows_, [window](const auto &win) { return win.id == window; });
  if (it != std::ranges::end(windows_)) {
    it->is_mapped = true;
  }
}

inline auto WindowManager::AddWindowToMapped(xcb_window_t window) -> WmWindow & {
  windows_.emplace_back(WmWindow{
      .id = window, .name = "", .is_mapped = true, .is_centered = false, .is_floating = false});
  return windows_.back();
}

#endif
