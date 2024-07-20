#include "ewmh.h"
#include "xcb_utils.h"
#include <cstring>
#include <optional>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

auto Ewmh::Init(xcb_connection_t *connection, int screen_number, xcb_screen_t *screen) -> Ewmh {
  INFO("Initializing EWMH atoms.");
  std::array<std::string, 56> atom_names{
      "_NET_SUPPORTED",
      "_NET_SUPPORTING_WM_CHECK",
      "_NET_WM_NAME",
      "_NET_WM_VISIBLE_NAME",
      "_NET_WM_MOVERESIZE",
      "_NET_WM_STATE_STICKY",
      "_NET_WM_STATE_FULLSCREEN",
      "_NET_WM_STATE_DEMANDS_ATTENTION",
      "_NET_WM_STATE_MODAL",
      "_NET_WM_STATE_HIDDEN",
      "_NET_WM_STATE_FOCUSED",
      "_NET_WM_STATE",
      "_NET_WM_WINDOW_TYPE",
      "_NET_WM_WINDOW_TYPE_NORMAL",
      "_NET_WM_WINDOW_TYPE_DOCK",
      "_NET_WM_WINDOW_TYPE_DIALOG",
      "_NET_WM_WINDOW_TYPE_UTILITY",
      "_NET_WM_WINDOW_TYPE_TOOLBAR",
      "_NET_WM_WINDOW_TYPE_SPLASH",
      "_NET_WM_WINDOW_TYPE_MENU",
      "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
      "_NET_WM_WINDOW_TYPE_POPUP_MENU",
      "_NET_WM_WINDOW_TYPE_TOOLTIP",
      "_NET_WM_WINDOW_TYPE_NOTIFICATION",
      "_NET_WM_DESKTOP",
      "_NET_WM_STRUT_PARTIAL",
      "_NET_CLIENT_LIST",
      "_NET_CLIENT_LIST_STACKING",
      "_NET_CURRENT_DESKTOP",
      "_NET_NUMBER_OF_DESKTOPS",
      "_NET_DESKTOP_NAMES",
      "_NET_DESKTOP_VIEWPORT",
      "_NET_ACTIVE_WINDOW",
      "_NET_CLOSE_WINDOW",
      "_NET_MOVERESIZE_WINDOW",
      "_NET_WM_USER_TIME",
      "_NET_STARTUP_ID",
      "_NET_WORKAREA",
      "_NET_WM_ICON",
      "WM_PROTOCOLS",
      "WM_DELETE_WINDOW",
      "UTF8_STRING",
      "WM_STATE",
      "WM_CLIENT_LEADER",
      "WM_TAKE_FOCUS",
      "WM_WINDOW_ROLE",
      "_NET_REQUEST_FRAME_EXTENTS",
      "_NET_FRAME_EXTENTS",
      "_MOTIF_WM_HINTS",
      "WM_CHANGE_STATE",
      "_NET_SYSTEM_TRAY_OPCODE",
      "_NET_SYSTEM_TRAY_COLORS",
      "_NET_SYSTEM_TRAY_VISUAL",
      "_NET_SYSTEM_TRAY_ORIENTATION",
      "_XEMBED_INFO",
      "MANAGER",
  };
  std::array<xcb_intern_atom_cookie_t, atom_names.size()> cookies{};
  for (std::size_t i = 0; i != atom_names.size(); ++i) {
    cookies[i] = xcb_intern_atom(connection, false, atom_names[i].size(), atom_names[i].c_str());
  }
  std::map<std::string, xcb_atom_t>         atoms{};
  std::array<xcb_atom_t, atom_names.size()> atom_values{};
  for (std::size_t i = 0; i != cookies.size(); ++i) {
    auto reply = xcb_intern_atom_reply(connection, cookies[i], nullptr);
    if (!reply) {
      ERROR("Failed to fetch atom \"{}\".", atom_names[i]);
      continue;
    }
    atoms[atom_names[i]] = reply->atom;
    atom_values[i]       = reply->atom;
  }
  INFO("Finished initializing EWMH atoms.");

  const std::string wm_name{"unknown_wm"};

  auto ewmh_window = xcb_generate_id(connection);
  xcb_create_window(connection, XCB_COPY_FROM_PARENT, ewmh_window, screen->root, -1, -1, 1, 1, 0,
                    XCB_WINDOW_CLASS_INPUT_ONLY, XCB_COPY_FROM_PARENT, XCB_NONE, nullptr);
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, ewmh_window,
                      atoms["_NET_SUPPORTING_WM_CHECK"], XCB_ATOM_WINDOW, 32, 1, &ewmh_window);
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, ewmh_window, atoms["_NET_WM_NAME"],
                      atoms["UTF8_STRING"], 8, wm_name.size(), wm_name.data());

  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, screen->root,
                      atoms["_NET_SUPPORTING_WM_CHECK"], XCB_ATOM_WINDOW, 32, 1, &ewmh_window);
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, screen->root, atoms["_NET_WM_NAME"],
                      atoms["UTF8_STRING"], 8, wm_name.size(), wm_name.data());
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, screen->root, atoms["_NET_SUPPORTED"],
                      XCB_ATOM_ATOM, 32, atom_values.size(), atom_values.data());
  xcb_delete_property(connection, screen->root, atoms["_NET_WORKAREA"]);

  return Ewmh(connection, std::move(atoms), ewmh_window, screen_number, screen);
}

Ewmh::Ewmh(xcb_connection_t *connection, std::map<std::string, xcb_atom_t> &&atoms,
           uint32_t ewmh_window, int screen_number, xcb_screen_t *screen)
    : connection_(connection), atoms_{std::move(atoms)}, ewmh_window_(ewmh_window),
      screen_number_(screen_number), screen_(screen), is_valid_(true) {}

[[nodiscard]] auto Ewmh::Property(const std::string &name, xcb_window_t window)
    -> std::unique_ptr<xcb_get_property_reply_t, decltype(&free)> {
  if (auto atom = Atom(name); atom) {
    const auto cookie = xcb_get_property(connection_, false, window, atom.value(),
                                         XCB_GET_PROPERTY_TYPE_ANY, 0, 128);
    auto       reply  = PtrWrap(xcb_get_property_reply(connection_, cookie, nullptr));
    if (!reply) {
      ERROR("Failed to fetch property: {}.", name);
      return PtrWrap<xcb_get_property_reply_t>(nullptr);
    }
    return reply;
  } else {
    ERROR("No such property name found: {}.", name);
  }
  return PtrWrap<xcb_get_property_reply_t>(nullptr);
}

void Ewmh::GrabICCCMSizeHints(WmWindow &window) {
  xcb_size_hints_t size_hints;
  auto             cookie = xcb_icccm_get_wm_normal_hints_unchecked(connection_, window.id);
  auto reply = xcb_icccm_get_wm_normal_hints_reply(connection_, cookie, &size_hints, nullptr);
  if (reply) {
    // const auto width =
    //     std::max(std::max(size_hints.width, window.default_size.x), size_hints.base_width);
    // const auto height =
    //     std::max(std::max(size_hints.height, window.default_size.y), size_hints.base_height);
    // window.pseudo_size = Vector2D(width, height);
  } else {
    ERROR("Failed to get ICCCM size hints.");
  }
}

void Ewmh::UpdateCurrentWindow(xcb_window_t window) {
  if (auto atom = Atom("_NET_ACTIVE_WINDOW"); atom) {
    xcb_change_property(connection_, XCB_PROP_MODE_REPLACE, screen_->root, atom.value(),
                        XCB_ATOM_WINDOW, 32, 1, &window);
  }
}

void Ewmh::UpdateWindow(xcb_window_t window, bool focused) {
  // if (auto atom = Atom("_NET_WM_DESKTOP"); atom) {
  //   xcb_change_property(connection_, XCB_PROP_MODE_REPLACE, window, atom.value(),
  //   XCB_ATOM_CARDINAL, 32, 1, )
  // }
  if (auto atom = Atom("WM_STATE"); atom) {
    std::array<uint32_t, 2> values{XCB_ICCCM_WM_STATE_NORMAL, XCB_NONE};
    xcb_change_property(connection_, XCB_PROP_MODE_REPLACE, window, atom.value(), atom.value(), 32,
                        2, values.data());
  }
  if (focused) {
    if (auto atom_state_focused = Atom("_NET_WM_STATE_FOCUSED"); atom_state_focused) {
      if (auto atom_state = Atom("_NET_WM_STATE"); atom_state) {
        std::array<uint32_t, 1> values{atom_state_focused.value()};
        xcb_change_property(connection_, XCB_PROP_MODE_APPEND, window, atom_state.value(),
                            XCB_ATOM_ATOM, 32, 1, values.data());
      }
    }
  }
}
