#pragma once

#include <map>
#include <unordered_map>
extern "C" {
#include <X11/Xlib.h>
}
#include <memory>

#define ERROR(msg) spdlog::error((msg))
#define INFO(msg) spdlog::info((msg))

#define ERRORF(msg, ...) ERROR(std::format((msg), __VA_ARGS__))
#define INFOF(msg, ...) INFO(std::format((msg), __VA_ARGS__))

#define CHECK(stmt)                                                            \
  {                                                                            \
    auto res = stmt;                                                           \
    if (!res) {                                                                \
      ERRORF("Failed check, error: '{}' at {}", res, #stmt);                   \
      throw std::runtime_error(#stmt);                                         \
    }                                                                          \
  }

class WindowManager {
public:
  static std::unique_ptr<WindowManager> Init();
  ~WindowManager();
  void Run();

private:
  explicit WindowManager(Display *display);
  void Frame(Window window, bool was_created_before_wm);
  void UnFrame(Window window);
  void RegisterInteractions(Window window);

  void OnConfigureNotify(const XConfigureEvent &event);
  void OnCreateNotify(const XCreateWindowEvent &event);
  void OnDestroyNotify(const XDestroyWindowEvent &event);
  void OnGravityNotify(const XGravityEvent &event);
  void OnMappingNotify(const XMappingEvent &event);
  void OnReparentNotify(const XReparentEvent &event);
  void OnUnmapNotify(const XUnmapEvent &event);
  void OnMapRequest(const XMapRequestEvent &event);
  void OnButtonPress(const XButtonEvent &event);
  void OnClientMessage(const XClientMessageEvent &event);
  void OnConfigureRequest(const XConfigureRequestEvent &event);
  void OnEnterNotify(const XCrossingEvent &event);
  void OnExpose(const XExposeEvent &event);
  void OnKeyPress(const XKeyEvent &event);
  void OnFocusIn(const XFocusChangeEvent &event);
  void OnMotionNotify(const XMotionEvent &event);
  void OnPropertyNotify(const XPropertyEvent &event);

  static int OnXError([[maybe_unused]] Display *display,
                      [[maybe_unused]] XErrorEvent *event);
  static int OnWMDetected([[maybe_unused]] Display *display,
                          [[maybe_unused]] XErrorEvent *event);

private:
  // Handle to the underlying XLib display struct
  Display *display_;
  // Handle to root window
  const Window root_;

  // Window and it's frame
  std::unordered_map<Window, Window> clients_;
};

inline static std::string XRequestCodeToString(int request_code) {
  static const char *const X_REQUEST_CODE_NAMES[] = {
      "",
      "CreateWindow",
      "ChangeWindowAttributes",
      "GetWindowAttributes",
      "DestroyWindow",
      "DestroySubwindows",
      "ChangeSaveSet",
      "ReparentWindow",
      "MapWindow",
      "MapSubwindows",
      "UnmapWindow",
      "UnmapSubwindows",
      "ConfigureWindow",
      "CirculateWindow",
      "GetGeometry",
      "QueryTree",
      "InternAtom",
      "GetAtomName",
      "ChangeProperty",
      "DeleteProperty",
      "GetProperty",
      "ListProperties",
      "SetSelectionOwner",
      "GetSelectionOwner",
      "ConvertSelection",
      "SendEvent",
      "GrabPointer",
      "UngrabPointer",
      "GrabButton",
      "UngrabButton",
      "ChangeActivePointerGrab",
      "GrabKeyboard",
      "UngrabKeyboard",
      "GrabKey",
      "UngrabKey",
      "AllowEvents",
      "GrabServer",
      "UngrabServer",
      "QueryPointer",
      "GetMotionEvents",
      "TranslateCoords",
      "WarpPointer",
      "SetInputFocus",
      "GetInputFocus",
      "QueryKeymap",
      "OpenFont",
      "CloseFont",
      "QueryFont",
      "QueryTextExtents",
      "ListFonts",
      "ListFontsWithInfo",
      "SetFontPath",
      "GetFontPath",
      "CreatePixmap",
      "FreePixmap",
      "CreateGC",
      "ChangeGC",
      "CopyGC",
      "SetDashes",
      "SetClipRectangles",
      "FreeGC",
      "ClearArea",
      "CopyArea",
      "CopyPlane",
      "PolyPoint",
      "PolyLine",
      "PolySegment",
      "PolyRectangle",
      "PolyArc",
      "FillPoly",
      "PolyFillRectangle",
      "PolyFillArc",
      "PutImage",
      "GetImage",
      "PolyText8",
      "PolyText16",
      "ImageText8",
      "ImageText16",
      "CreateColormap",
      "FreeColormap",
      "CopyColormapAndFree",
      "InstallColormap",
      "UninstallColormap",
      "ListInstalledColormaps",
      "AllocColor",
      "AllocNamedColor",
      "AllocColorCells",
      "AllocColorPlanes",
      "FreeColors",
      "StoreColors",
      "StoreNamedColor",
      "QueryColors",
      "LookupColor",
      "CreateCursor",
      "CreateGlyphCursor",
      "FreeCursor",
      "RecolorCursor",
      "QueryBestSize",
      "QueryExtension",
      "ListExtensions",
      "ChangeKeyboardMapping",
      "GetKeyboardMapping",
      "ChangeKeyboardControl",
      "GetKeyboardControl",
      "Bell",
      "ChangePointerControl",
      "GetPointerControl",
      "SetScreenSaver",
      "GetScreenSaver",
      "ChangeHosts",
      "ListHosts",
      "SetAccessControl",
      "SetCloseDownMode",
      "KillClient",
      "RotateProperties",
      "ForceScreenSaver",
      "SetPointerMapping",
      "GetPointerMapping",
      "SetModifierMapping",
      "GetModifierMapping",
      "NoOperation",
  };
  return X_REQUEST_CODE_NAMES[request_code];
}
