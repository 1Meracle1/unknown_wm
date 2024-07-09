#pragma once

#include "logging.h"
#include <string>

enum class Keys {
  Super,
  Alt,
  Shift,
  Ctrl,

  Left,
  Right,
  Up,
  Down,

  Plus,
  Minus,

  Number_1,
  Number_2,
  Number_3,
  Number_4,
  Number_5,
  Number_6,
  Number_7,
  Number_8,
  Number_9
};

// constexpr inline auto XcbKey(Keys key) {
// XCB_MOD_MASK_
// }

constexpr inline auto ToString(Keys key) {
  const char *res;
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
  }
  return res;
}
