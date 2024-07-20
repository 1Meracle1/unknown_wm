#ifndef EWMH_H
#define EWMH_H

#include "logging.h"
#include "xcb_utils.h"
#include <compare>
#include <map>
#include <memory>
#include <optional>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

static const std::string g_wm_class              = "wm";
static const std::string g_motion_recorder_class = "motion_recorder";
static const std::string g_wm_name               = "unknown_wm";

struct MotionRecorder {
  xcb_window_t id;
  uint16_t     sequence;
  bool         enabled;
};

struct WmWindow {
  xcb_window_t id;
  std::string  name;
  bool         is_mapped;
  bool         is_centered;
  bool         is_floating;
  bool         is_dirty;
  Vector2D     position;
  Vector2D     size;
};

class Ewmh {
public:
  static auto Init(xcb_connection_t *connection, int screen_number, xcb_screen_t *screen) -> Ewmh;
  [[nodiscard]] static inline auto Instance() -> Ewmh & { return instance_; }
  Ewmh()                                            = default;
  Ewmh(Ewmh &&)                                     = default;
  Ewmh(const Ewmh &)                                = delete;
  Ewmh                     &operator=(const Ewmh &) = delete;
  Ewmh                     &operator=(Ewmh &&)      = default;
  [[nodiscard]] inline auto IsValid() const -> bool { return is_valid_; }

  [[nodiscard]] inline auto Atoms() const -> const std::map<std::string, xcb_atom_t> & {
    return atoms_;
  }
  [[nodiscard]] inline auto Atom(const std::string &name) const -> std::optional<xcb_atom_t> {
    if (atoms_.contains(name)) {
      return std::optional<xcb_atom_t>{atoms_.at(name)};
    }
    return std::nullopt;
  }

  [[nodiscard]] auto Property(const std::string &name, xcb_window_t window)
      -> std::unique_ptr<xcb_get_property_reply_t, decltype(&free)>;

  void GrabICCCMSizeHints(WmWindow &window);

  void UpdateCurrentWindow(xcb_window_t window);
  void UpdateWindow(xcb_window_t window, bool focused);

private:
  explicit Ewmh(xcb_connection_t *connection, std::map<std::string, xcb_atom_t> &&atoms,
                uint32_t ewmh_window, int screen_number, xcb_screen_t *screen);

private:
  static Ewmh instance_;
  bool        is_valid_{false};

  xcb_connection_t                 *connection_{nullptr}; // non-owning
  std::map<std::string, xcb_atom_t> atoms_{};

  uint32_t      ewmh_window_{0};
  int           screen_number_{0};
  xcb_screen_t *screen_{nullptr};
};

#endif
