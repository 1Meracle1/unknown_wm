#pragma once

#include "logging.h"
#include <X11/X.h>
#include <X11/Xutil.h>
#include <string>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

namespace Keys {
constexpr uint32_t Super = XCB_MOD_MASK_4;
constexpr uint32_t Alt = XCB_MOD_MASK_1;
constexpr uint32_t Shift = XCB_MOD_MASK_SHIFT;
constexpr uint32_t Ctrl = XCB_MOD_MASK_CONTROL;
constexpr uint32_t CapsLock = XCB_MOD_MASK_LOCK;
constexpr uint32_t ScrollLock = XCB_MOD_MASK_5;
constexpr uint32_t NumLock = XCB_MOD_MASK_2;

constexpr uint32_t Return = XK_Return;

constexpr uint32_t Left = XK_Left;
constexpr uint32_t Right = XK_Right;
constexpr uint32_t Up = XK_Up;
constexpr uint32_t Down = XK_Down;

constexpr uint32_t Plus = XK_plus;
constexpr uint32_t Minus = XK_minus;

constexpr uint32_t Number_1 = XK_1;
constexpr uint32_t Number_2 = XK_2;
constexpr uint32_t Number_3 = XK_3;
constexpr uint32_t Number_4 = XK_4;
constexpr uint32_t Number_5 = XK_5;
constexpr uint32_t Number_6 = XK_6;
constexpr uint32_t Number_7 = XK_7;
constexpr uint32_t Number_8 = XK_8;
constexpr uint32_t Number_9 = XK_9;
}; // namespace Keys

struct Key {
  uint32_t modifier;
  uint32_t key_symbol;
};

constexpr inline auto ToString(uint32_t key) {
  std::string res;
  switch (key) {
  case Keys::Super:
    res = "Super";
    break;
  case Keys::Alt:
    res = "Alt";
    break;
  case Keys::Shift:
    res = "Shift";
    break;
  case Keys::Left:
    res = "Left";
    break;
  case Keys::Right:
    res = "Right";
    break;
  case Keys::Up:
    res = "Up";
    break;
  case Keys::Down:
    res = "Down";
    break;
  case Keys::Number_1:
    res = "Number_1";
    break;
  case Keys::Number_2:
    res = "Number_2";
    break;
  case Keys::Number_3:
    res = "Number_3";
    break;
  case Keys::Number_4:
    res = "Number_4";
    break;
  case Keys::Number_5:
    res = "Number_5";
    break;
  case Keys::Number_6:
    res = "Number_6";
    break;
  case Keys::Number_7:
    res = "Number_7";
    break;
  case Keys::Number_8:
    res = "Number_8";
    break;
  case Keys::Number_9:
    res = "Number_9";
    break;
  case Keys::Ctrl:
    res = "Ctrl";
    break;
  case Keys::Plus:
    res = "Plus";
    break;
  case Keys::Minus:
    res = "Minus";
    break;
  case Keys::Return:
    res = "Return";
    break;
  default:
    res = std::to_string(key);
  }
  return res;
}
