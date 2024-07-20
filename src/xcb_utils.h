#ifndef XCB_UTILS_H
#define XCB_UTILS_H

#include "type_utils.h"
#include <xcb/xcb.h>
#include <utility>
#include <xcb/xcb_ewmh.h>

template <typename ReplyF, typename... Args>
constexpr inline auto XcbReply(ReplyF &&reply_func, Args &&...params) {
  xcb_generic_error_t *error;
  return std::make_pair(std::forward<ReplyF>(reply_func)(std::forward<Args>(params)..., &error),
                        error == nullptr);
}
template <typename RequestF, typename ReplyF, typename... Args>
constexpr inline auto XcbReply(xcb_connection_t *connection, RequestF &&request_func,
                               ReplyF &&reply_func, Args &&...params) {
  xcb_generic_error_t *error;
  return std::make_pair(
      std::forward<ReplyF>(reply_func)(
          connection,
          std::forward<RequestF>(request_func)(connection, std::forward<Args>(params)...), &error),
      error == nullptr);
}
template <typename RequestF, typename ReplyF, typename... Args>
inline auto XcbReplyWrapPtr(xcb_connection_t *connection, RequestF &&request_func,
                            ReplyF &&reply_func, Args &&...params) {
  return PtrWrap(XcbReply<RequestF, ReplyF, Args...>(
      connection, std::forward<RequestF>(request_func), std::forward<ReplyF>(reply_func),
      std::forward<Args>(params)...));
}

template <typename ReplyT, typename RequestF, typename ReplyF, typename... Args>
constexpr inline auto XcbEwmhReply(xcb_ewmh_connection_t &connection, RequestF &&request_func,
                                   ReplyF &&reply_func, Args &&...params) {
  auto cookie = std::forward<RequestF>(request_func)(&connection, std::forward<Args>(params)...);
  xcb_generic_error_t *error;
  ReplyT               res;
  auto                 is_ok = std::forward<ReplyF>(reply_func)(&connection, cookie, &res, &error);
  return std::make_pair(res, is_ok == 1 && error == nullptr);
}

#endif
