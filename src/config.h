#pragma once

#include <X11/X.h>
#include <X11/Xutil.h>
#include <cstdint>
#include <map>
#include <variant>
#include <vector>

enum class Action {
  Move,
  Kill,
  Switch,
};

struct Button {
  uint32_t modifier;
  uint32_t button;
};

struct Key {
  uint32_t modifier;
  uint32_t button;
};

struct InteractionDescription {
  std::variant<Button, Key> condition;
  Action expected_action;
};

const uint32_t g_modifier_key = Mod1Mask;

const std::vector<InteractionDescription> g_interactions{
    {Button{g_modifier_key, Button1}, Action::Move},
    {Button{g_modifier_key, Button3}, Action::Move},
    {Key{g_modifier_key, XK_F4}, Action::Kill},
    {Key{g_modifier_key, XK_Tab}, Action::Switch},
};

const uint64_t g_border_width = 3;
const uint64_t g_border_color = 0xff0000;
const uint64_t g_bg_color = 0x0000ff;
