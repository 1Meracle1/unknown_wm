#pragma once

#include "connection.h"
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
             Connection &connection) { func(connection, args...); };
}

inline void WindowResize(Connection &connection, int x_amount, int y_amount) {
  DEBUG("Action: WindowResize");
}

inline void WindowMove(Connection &connection, int x_amount, int y_amount) {
  DEBUG("Action: WindowMove");
}

inline void WindowFocus(Connection &connection, int x_amount, int y_amount) {
  DEBUG("Action: WindowFocus");
}

inline void WorkspaceFocus(Connection &connection, uint32_t workspace_id) {
  DEBUG("Action: WorkspaceFocus");
}

inline void WorkspaceWindowMove(Connection &connection, uint32_t workspace_id) {
  DEBUG("Action: WorkspaceWindowMove");
}

using ActionType = std::function<void(Connection &)>;
