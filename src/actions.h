#pragma once

#include "connection.h"
#include "event.h"
#include "logging.h"
#include <functional>

template <typename T> struct FunctionTrait;

template <typename RetT, typename... Args> struct FunctionTrait<RetT(Args...)> {
  static constexpr std::size_t args_count = sizeof...(Args);
};

/* Example:
template<typename F>
constexpr void PrintArgCount() {
  constexpr auto count = function_arg_count_v<F>;
  INFOF("Arg count: {}", count);
}

void add0(){}
void add1(int a){}
void add2(int a, double b){}

PrintArgCount<decltype(add0)>();
PrintArgCount<decltype(add1)>();
PrintArgCount<decltype(add2)>();
*/
template <typename T>
inline constexpr std::size_t function_arg_count_v =
    FunctionTrait<T>::args_count;

template <typename F, typename... Args>
constexpr inline auto Action(F &&func, Args &&...args) {
  return [func = std::forward<F>(func), ... args = std::forward<Args>(args)](
             EventHandler &event_handler) { func(event_handler, args...); };
}

inline void WindowResize(EventHandler &event_handler, int x_amount,
                         int y_amount) {
  event_handler.WindowResize(x_amount, y_amount);
}

inline void WindowMove(EventHandler &event_handler, int x_amount,
                       int y_amount) {
  event_handler.WindowMove(x_amount, y_amount);
}

inline void WindowFocus(EventHandler &event_handler, int x_amount,
                        int y_amount) {
  event_handler.WindowFocus(x_amount, y_amount);
}

inline void WorkspaceFocus(EventHandler &event_handler, uint32_t workspace_id) {
  event_handler.WorkspaceFocus(workspace_id);
}

inline void WorkspaceWindowMove(EventHandler &event_handler,
                                uint32_t workspace_id) {
  event_handler.WorkspaceWindowMove(workspace_id);
}

using ActionType = std::function<void(EventHandler &)>;
