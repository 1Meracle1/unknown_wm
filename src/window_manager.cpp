#include "window_manager.h"
#include "config.h"
#include "connection.h"
#include "logging.h"

#include <X11/X.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
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
    : connection_{std::move(connection)} {}

void WindowManager::Run() {
  RegisterWM();

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

  // reparent existing windows
  auto parent_windows = connection_->ListMappedWindows();
  INFOF("Parent windows: {}", parent_windows.size());
  for (const auto &[win, screen_id] : parent_windows) {
    connection_->ChangeWindowAttributesChecked(
        win, XCB_CW_EVENT_MASK,
        XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY);
    connection_->ConfigureWindow(win, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                                 BORDER_WIDTH);
    connection_->ChangeWindowAttributesChecked(win, XCB_CW_BORDER_PIXEL,
                                               BORDER_COLOR_INACTIVE);
  }

  if (auto focused_win_opt = connection_->FocusedWindow(); focused_win_opt) {
    auto focused_win = focused_win_opt.value();
    INFO("Non-root focused window found, reapplying focus on it");
    connection_->ConfigureWindow(focused_win,
                                 XCB_CONFIG_WINDOW_BORDER_WIDTH |
                                     XCB_CONFIG_WINDOW_STACK_MODE,
                                 BORDER_WIDTH, XCB_STACK_MODE_ABOVE);
    connection_->SetInputFocus(XCB_NONE, XCB_NONE);
    connection_->SetInputFocus(XCB_INPUT_FOCUS_PARENT, focused_win);
  }
}

void WindowManager::RegisterWM() {
  INFO("Registering WM events on screens roots");
  for (const auto screen : connection_->GetScreens()) {
    connection_->ChangeWindowAttributesChecked(
        screen->root, XCB_CW_EVENT_MASK,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT);
  }
}
