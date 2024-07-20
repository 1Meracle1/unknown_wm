#include "monitor.h"
#include "logging.h"
#include "type_utils.h"
#include "types.h"
#include "xcb_utils.h"
#include <vector>
#include <xcb/randr.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

namespace {
std::vector<Monitor> monitors_;
}; // namespace

auto SetupMonitors(xcb_connection_t *connection, xcb_window_t root) -> bool {
  monitors_.clear();

  auto extension = xcb_get_extension_data(connection, &xcb_randr_id);
  if (!extension->present) {
    ERROR("Randr extension is not present.");
    return false;
  }
  g_randr_event_base = extension->first_event;

  if (auto [randr_version, is_ok] =
          XcbReply(connection, xcb_randr_query_version, xcb_randr_query_version_reply,
                   XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
      !is_ok) {
    ERROR("Failed to find matching Randr version, requested {}.{}.", XCB_RANDR_MAJOR_VERSION,
          XCB_RANDR_MINOR_VERSION);
    return false;
  }

  INFO("Setting up Randr.");
  auto [monitors, is_ok] =
      XcbReplyWrapPtr(connection, xcb_randr_get_monitors, xcb_randr_get_monitors_reply, root, true);
  if (!is_ok) {
    ERROR("Failed to retrieve monitors.");
    return false;
  }
  auto monitors_num = xcb_randr_get_monitors_monitors_length(monitors.get());
  if (monitors_num < 1) {
    ERROR("Randr returned incorrect number of monitors: {}.", monitors_num);
    return false;
  }
  for (auto it = xcb_randr_get_monitors_monitors_iterator(monitors.get()); it.rem;
       xcb_randr_monitor_info_next(&it)) {
    const auto monitor_info = it.data;
    auto [atom_name, is_ok] =
        XcbReplyWrapPtr(connection, xcb_get_atom_name, xcb_get_atom_name_reply, monitor_info->name);
    if (!is_ok) {
      ERROR("Failed to get monitor info.");
      continue;
    }

    monitors_.emplace_back(
        Monitor{.name     = std::string(xcb_get_atom_name_name(atom_name.get()),
                                        xcb_get_atom_name_name_length(atom_name.get())),
                .primary  = monitor_info->primary == 1,
                .position = Vector2D{monitor_info->x, monitor_info->y},
                .size     = Vector2D{monitor_info->width, monitor_info->height}});
    INFO("Monitor found: name: {}, primary: {}.", monitors_.back().name, monitors_.back().primary);
  }
  if (monitors_.empty()) {
    ERROR("Failed to grab any monitors.");
    return false;
  }

  return true;
}

auto GetMonitorFromPos(xcb_connection_t *connection, Vector2D &&pos) -> const Monitor & {
  assert(!monitors_.empty());
  auto it = std::ranges::find_if(monitors_,
                                 [position = std::forward<Vector2D>(pos)](const auto &monitor) {
                                   return IsInRect(position, monitor.position, monitor.size);
                                 });
  if (it == std::ranges::end(monitors_)) {
    return monitors_.front();
  }
  return *it;
}
