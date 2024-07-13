#include "window_manager.h"
#include "config.h"
#include "connection.h"
#include "event.h"
#include "logging.h"

#include <X11/X.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include <csignal>
#include <memory>

auto WindowManager::Init() -> std::unique_ptr<WindowManager> {
  auto connection = Connection::Init();
  if (!connection) {
    return nullptr;
  }
  return std::unique_ptr<WindowManager>(
      new WindowManager(std::move(connection)));
}

WindowManager::WindowManager(std::unique_ptr<Connection> connection)
    : connection_{std::move(connection)}, event_handler_{*connection_} {}

void WindowManager::Run() {
  // do cleanup of zombie processes
  {
    struct sigaction sa = {};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);
    while (waitpid(-1, nullptr, WNOHANG) > 0)
      ;
  }

  {
    xcb_ewmh_connection_t ewmh{};
    auto cookie = xcb_ewmh_init_atoms(connection_->Get(), &ewmh);

    xcb_generic_error_t *error;
    if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, &error)) {
      ERROR("Failed to init WM atoms: {}", error->error_code);
      free(error);
    }
    INFO("Screens fetched: {}", ewmh.nb_screens);

    for (int i = 0; i < ewmh.nb_screens; ++i) {
      auto virtual_roots_cookie = xcb_ewmh_get_virtual_roots(&ewmh, i);
      xcb_ewmh_get_windows_reply_t windows_reply;
      if (!xcb_ewmh_get_virtual_roots_reply(&ewmh, virtual_roots_cookie,
                                            &windows_reply, &error)) {
        ERROR("Failed to get virtual roots reply");
        if (error) {
          ERROR("Error: {}", error->error_code);
          free(error);
        }
      }
      INFO("Virtual roots fetched: {}", windows_reply.windows_len);

      auto get_client_list_cookie = xcb_ewmh_get_client_list(&ewmh, i);
      xcb_ewmh_get_windows_reply_t client_windows_reply;
      if (!xcb_ewmh_get_client_list_reply(&ewmh, get_client_list_cookie,
                                          &client_windows_reply, &error)) {
        ERROR("Failed to client list");
      }
      INFO("Client windows fetched: {}", client_windows_reply.windows_len);

      auto get_property_cookie = xcb_ewmh_get_supported(&ewmh, i);
      xcb_ewmh_get_atoms_reply_t get_supported_reply;
      if (!xcb_ewmh_get_supported_reply(&ewmh, get_property_cookie,
                                        &get_supported_reply, &error)) {
        if (error) {
          ERROR("Failed to get supported properties for screen {}: {}", i,
                error->error_code);
          free(error);
        } else {
          ERROR("Failed to get supported properties for screen {}", i);
        }
      }
      xcb_get_property_reply_t get_property_reply;
      uint8_t number_atoms = xcb_ewmh_get_supported_from_reply(
          &get_supported_reply, &get_property_reply);
      INFO("Fetched atoms: {} for screen: {}", number_atoms, i);
    }

    xcb_ewmh_connection_wipe(&ewmh);
  }

  // INFO("Registering WM events on screens roots");
  // connection_->SubscribeWMEvents();

  // // reparent existing windows
  // auto parent_windows = connection_->ListMappedWindows();
  // INFO("Parent windows: {}", parent_windows.size());
  // for (const auto &[win, screen_id] : parent_windows) {
  //   connection_->ChangeWindowAttributesChecked(
  //       win, XCB_CW_EVENT_MASK,
  //       XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
  //           XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);
  //   connection_->ConfigureWindow(win, XCB_CONFIG_WINDOW_BORDER_WIDTH,
  //                                BORDER_WIDTH);
  //   connection_->ChangeWindowAttributesChecked(win, XCB_CW_BORDER_PIXEL,
  //                                              BORDER_COLOR_INACTIVE);
  // }

  // // reapply focus to a previously focused window
  // if (auto focused_win_opt = connection_->FocusedWindow(); focused_win_opt) {
  //   auto focused_win = focused_win_opt.value();
  //   INFO("Non-root focused window found, reapplying focus on it");
  //   connection_->ConfigureWindow(focused_win,
  //                                XCB_CONFIG_WINDOW_BORDER_WIDTH |
  //                                    XCB_CONFIG_WINDOW_STACK_MODE,
  //                                BORDER_WIDTH, XCB_STACK_MODE_ABOVE);
  //   connection_->SetInputFocus(XCB_NONE, XCB_NONE);
  //   connection_->SetInputFocus(XCB_INPUT_FOCUS_PARENT, focused_win);
  // }

  // INFO("Registering keybindings");
  // connection_->RegisterKeybindings();

  // INFO("Starting event loop");
  // // listen and process events
  // for (;;) {
  //   connection_->Flush();
  //   if (auto error = xcb_connection_has_error(connection_->Get()); error !=
  //   0) {
  //     ERROR("Encountered connection error: {}", error);
  //     break;
  //   }

  //   auto event = connection_->WaitForEvent();
  //   if (!event) {
  //     ERROR("Received nullptr event");
  //     continue;
  //   }
  //   auto event_type = XCB_EVENT_RESPONSE_TYPE(event.get());
  //   event_handler_.DispatchEvent(event_type, std::move(event));
  // }
}
