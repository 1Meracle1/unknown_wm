#ifndef TYPE_UTILS_H
#define TYPE_UTILS_H

#include "types.h"
#include <memory>

template <typename T> class Cptr {
public:
  explicit Cptr() : data_{nullptr} {};
  explicit Cptr(T *data) : data_{data} {};
  Cptr(Cptr &&other)                 = default;
  Cptr &operator=(Cptr &&other)      = default;
  Cptr(const Cptr &other)            = delete;
  Cptr &operator=(const Cptr &other) = delete;
  ~Cptr() { free(data_); }

  explicit operator bool() const { return data_ == nullptr; }
  T       *operator->() { return data_; }
  const T *operator->() const { return data_; }

private:
  T *data_;
};

template <typename T> inline auto PtrWrap(T *ptr) {
  return std::unique_ptr<T, decltype(&free)>{ptr, &free};
}
template <typename T, typename A> inline auto PtrWrap(std::pair<T *, A> &&pair) {
  return std::make_pair(std::unique_ptr<T, decltype(&free)>{pair.first, &free}, pair.second);
}

template <typename T> struct FunctionTrait;
template <typename RetT, typename... Args> struct FunctionTrait<RetT(Args...)> {
  static constexpr std::size_t args_count = sizeof...(Args);
  using ReturnType                        = RetT;
};
template <typename T>
inline constexpr std::size_t function_arg_count_v = FunctionTrait<T>::args_count;

inline constexpr auto IsInRect(const Vector2D &pos, const Vector2D &top_left,
                               const Vector2D &size) {
  return pos.x >= top_left.x && pos.x <= top_left.x + size.x && pos.y >= top_left.y &&
         pos.y <= top_left.y + size.y;
}

#endif
