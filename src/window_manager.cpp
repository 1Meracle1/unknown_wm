#include "window_manager.h"
#include "config.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include <cassert>
#include <mutex>
#include <variant>

static bool g_wm_detected;
static std::mutex g_wm_detected_mutex;

std::unique_ptr<WindowManager> WindowManager::Init() {
  const char *display_c_str = nullptr;
  Display *display = XOpenDisplay(display_c_str);
  if (display == nullptr) {
    return nullptr;
  }
  return std::unique_ptr<WindowManager>(new WindowManager(display));
}

WindowManager::WindowManager(Display *display)
    : display_{display}, root_{DefaultRootWindow(display_)} {}

WindowManager::~WindowManager() { XCloseDisplay(display_); }

void WindowManager::Run() {
  {
    std::lock_guard<std::mutex> lock{g_wm_detected_mutex};
    g_wm_detected = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);

    XSelectInput(display_, root_,
                 SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(display_, false);
    if (g_wm_detected) {
      ERRORF("Detected another window managed on display {}",
             XDisplayString(display_));
      return;
    }
  }

  XSetErrorHandler(&WindowManager::OnXError);
  XSync(display_, false);

  XGrabServer(display_);
  {
    // reparent existing windows
    Window returned_root;
    Window returned_parent;
    Window *top_level_windows;
    uint32_t num_top_level_windows;
    CHECK(XQueryTree(display_, root_, &returned_root, &returned_parent,
                     &top_level_windows, &num_top_level_windows));
    for (uint32_t i = 0; i < num_top_level_windows; i++) {
      Frame(top_level_windows[i], true);
    }
    XFree(top_level_windows);
  }
  XUngrabServer(display_);

  while (true) {
    XEvent event;
    XNextEvent(display_, &event);
    INFOF("Received event {}", event.type);
    switch (event.type) {
    case CreateNotify:
      OnCreateNotify(event.xcreatewindow);
      break;
    case DestroyNotify:
      OnDestroyNotify(event.xdestroywindow);
      break;
    case ConfigureNotify:
      OnConfigureNotify(event.xconfigure);
      break;
    case ConfigureRequest:
      OnConfigureRequest(event.xconfigurerequest);
      break;
    case EnterNotify:
      OnEnterNotify(event.xcrossing);
      break;
    case Expose:
      OnExpose(event.xexpose);
      break;
    case FocusIn:
      OnFocusIn(event.xfocus);
      break;
    case UnmapNotify:
      OnUnmapNotify(event.xunmap);
      break;
    case MappingNotify:
      OnMappingNotify(event.xmapping);
      break;
    case MapRequest:
      OnMapRequest(event.xmaprequest);
      break;
    case ButtonPress:
      OnButtonPress(event.xbutton);
      break;
    case KeyPress:
      OnKeyPress(event.xkey);
      break;
    case MotionNotify:
      OnMotionNotify(event.xmotion);
      break;
    case ClientMessage:
      OnClientMessage(event.xclient);
      break;
    case PropertyNotify:
      OnPropertyNotify(event.xproperty);
      break;
    default:
      INFOF("Skipped event {}", event.type);
    }
  }
}

void WindowManager::OnConfigureNotify(const XConfigureEvent &event) {}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent &event) {
  XWindowChanges changes;
  changes.x = event.x;
  changes.y = event.y;
  changes.width = event.width;
  changes.height = event.height;
  changes.border_width = event.border_width;
  changes.sibling = event.above;
  changes.stack_mode = event.detail;
  if (clients_.contains(event.window)) {
    const Window frame = clients_[event.window];
    XConfigureWindow(display_, frame, event.value_mask, &changes);
  }
  XConfigureWindow(display_, event.window, event.value_mask, &changes);
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent &event) {}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent &event) {}

void WindowManager::OnGravityNotify(const XGravityEvent &event) {}

void WindowManager::OnMappingNotify(const XMappingEvent &event) {}

void WindowManager::OnReparentNotify(const XReparentEvent &event) {}

void WindowManager::OnUnmapNotify(const XUnmapEvent &event) {
  if (!clients_.contains(event.window)) {
    INFO("Ignore UnmapNotify for non-client window");
    return;
  }
  if (event.event == root_) {
    INFO("Ignore UnmapNotify for reparented pre-existing window");
    return;
  }
  UnFrame(event.window);
}

void WindowManager::OnMapRequest(const XMapRequestEvent &event) {
  Frame(event.window, false);
  XMapWindow(display_, event.window);
}

void WindowManager::OnButtonPress(const XButtonEvent &event) {}

void WindowManager::OnClientMessage(const XClientMessageEvent &event) {}

void WindowManager::OnEnterNotify(const XCrossingEvent &event) {}

void WindowManager::OnExpose(const XExposeEvent &event) {}

void WindowManager::OnKeyPress(const XKeyEvent &event) {}

void WindowManager::OnFocusIn(const XFocusChangeEvent &event) {}

void WindowManager::OnMotionNotify(const XMotionEvent &event) {}

void WindowManager::OnPropertyNotify(const XPropertyEvent &event) {}

void WindowManager::Frame(Window window, bool was_created_before_wm) {
  if (clients_.contains(window))
    return;

  XWindowAttributes x_window_attrs;
  CHECK(XGetWindowAttributes(display_, window, &x_window_attrs));
  if (was_created_before_wm) {
    // don't touch if window is not visible
    if (x_window_attrs.override_redirect ||
        x_window_attrs.map_state != IsViewable) {
      return;
    }
  }

  // create frame
  const Window frame = XCreateSimpleWindow(
      display_, root_, x_window_attrs.x, x_window_attrs.y, x_window_attrs.width,
      x_window_attrs.height, g_border_width, g_border_color, g_bg_color);
  CHECK(XSelectInput(display_, frame,
                     SubstructureRedirectMask | SubstructureNotifyMask));
  CHECK(XAddToSaveSet(display_, window));
  CHECK(XReparentWindow(display_, window, frame, 0, 0));
  CHECK(XMapWindow(display_, frame));
  clients_[window] = frame;

  RegisterInteractions(window);
  INFOF("Framed window {}", window);
}

void WindowManager::UnFrame(Window window) {
  if (clients_.count(window) == 0)
    return;

  const Window frame = clients_[window];
  XUnmapWindow(display_, frame);
  XReparentWindow(display_, window, root_, 0, 0);
  clients_.erase(window);
  INFOF("Unframed window {}", window);
}

void WindowManager::RegisterInteractions(Window window) {
  for (const auto &inter : g_interactions) {
    if (std::holds_alternative<Button>(inter.condition)) {
      auto button = std::get<Button>(inter.condition);
      CHECK(XGrabButton(display_, button.button, button.modifier, window, false,
                        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                        GrabModeAsync, GrabModeAsync, None, None));
      break;
    }
    if (std::holds_alternative<Key>(inter.condition)) {
      auto key = std::get<Key>(inter.condition);
      CHECK(XGrabKey(display_, XKeysymToKeycode(display_, key.button),
                     key.modifier, window, false, GrabModeAsync,
                     GrabModeAsync));
      break;
    }
  }
}

int WindowManager::OnXError([[maybe_unused]] Display *display,
                            [[maybe_unused]] XErrorEvent *event) {
  const int max_error_text_length = 1024;
  char error_text[max_error_text_length];
  XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
  ERRORF("Received X error: Request: {}, Error code: {}, Error text: {}",
         XRequestCodeToString(event->request_code), event->error_code,
         error_text);
}

int WindowManager::OnWMDetected([[maybe_unused]] Display *display,
                                [[maybe_unused]] XErrorEvent *event) {
  assert(static_cast<int>(event->error_code) == BadAccess);
  g_wm_detected = true;
  return 0;
}
