#pragma once

#include "actions.h"
#include "keys.h"
#include <cstdint>
#include <map>
#include <vector>

constexpr auto BORDER_WIDTH = 3U;
constexpr auto BORDER_COLOR_ACTIVE = 0xff666666;
constexpr auto BORDER_COLOR_INACTIVE = 0xffffffff;
constexpr auto BG_COLOR = 0x0000ff;

template <typename F, typename K>
constexpr auto Keybinding(F &&f, K modifier, K key_symbol) {
  return std::pair{Key{modifier, key_symbol}, std::forward<F>(f)};
}

const std::vector<std::pair<Key, ActionType>> KEYBINDINGS{
    Keybinding(Action(WindowMove, +10, 0), Keys::Alt | Keys::Shift,
               Keys::Right),
    Keybinding(Action(WindowMove, -10, 0), Keys::Alt | Keys::Shift, Keys::Left),
    Keybinding(Action(WindowMove, 0, +10), Keys::Alt | Keys::Shift, Keys::Up),
    Keybinding(Action(WindowMove, 0, -10), Keys::Alt | Keys::Shift, Keys::Down),

    // resize based on the bottom right corner of the window
    Keybinding(Action(WindowResize, 0, +10), Keys::Alt | Keys::Ctrl,
               Keys::Down),
    Keybinding(Action(WindowResize, 0, -10), Keys::Alt | Keys::Ctrl, Keys::Up),
    Keybinding(Action(WindowResize, +10, 0), Keys::Alt | Keys::Ctrl,
               Keys::Right),
    Keybinding(Action(WindowResize, -10, 0), Keys::Alt | Keys::Ctrl,
               Keys::Left),

    // resize in all the four sides
    Keybinding(Action(WindowResize, +10, +10), Keys::Alt, Keys::Plus),
    Keybinding(Action(WindowResize, -10, -10), Keys::Alt, Keys::Minus),

    // focus change
    Keybinding(Action(WindowFocus, +1, 0), Keys::Alt, Keys::Right),
    Keybinding(Action(WindowFocus, -1, 0), Keys::Alt, Keys::Left),
    Keybinding(Action(WindowFocus, 0, +1), Keys::Alt, Keys::Up),
    Keybinding(Action(WindowFocus, 0, -1), Keys::Alt, Keys::Down),

    // workspace focus change
    Keybinding(Action(WorkspaceFocus, 1), Keys::Alt, Keys::Number_1),
    Keybinding(Action(WorkspaceFocus, 2), Keys::Alt, Keys::Number_2),
    Keybinding(Action(WorkspaceFocus, 3), Keys::Alt, Keys::Number_3),
    Keybinding(Action(WorkspaceFocus, 4), Keys::Alt, Keys::Number_4),
    Keybinding(Action(WorkspaceFocus, 5), Keys::Alt, Keys::Number_5),
    Keybinding(Action(WorkspaceFocus, 6), Keys::Alt, Keys::Number_6),
    Keybinding(Action(WorkspaceFocus, 7), Keys::Alt, Keys::Number_7),
    Keybinding(Action(WorkspaceFocus, 8), Keys::Alt, Keys::Number_8),
    Keybinding(Action(WorkspaceFocus, 9), Keys::Alt, Keys::Number_9),

    // move window to a workspace
    Keybinding(Action(WorkspaceWindowMove, 1), Keys::Alt | Keys::Shift,
               Keys::Number_1),
    Keybinding(Action(WorkspaceWindowMove, 2), Keys::Alt | Keys::Shift,
               Keys::Number_2),
    Keybinding(Action(WorkspaceWindowMove, 3), Keys::Alt | Keys::Shift,
               Keys::Number_3),
    Keybinding(Action(WorkspaceWindowMove, 4), Keys::Alt | Keys::Shift,
               Keys::Number_4),
    Keybinding(Action(WorkspaceWindowMove, 5), Keys::Alt | Keys::Shift,
               Keys::Number_5),
    Keybinding(Action(WorkspaceWindowMove, 6), Keys::Alt | Keys::Shift,
               Keys::Number_6),
    Keybinding(Action(WorkspaceWindowMove, 7), Keys::Alt | Keys::Shift,
               Keys::Number_7),
    Keybinding(Action(WorkspaceWindowMove, 8), Keys::Alt | Keys::Shift,
               Keys::Number_8),
    Keybinding(Action(WorkspaceWindowMove, 9), Keys::Alt | Keys::Shift,
               Keys::Number_9),
};
