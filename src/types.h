#ifndef TYPES_H
#define TYPES_H

#include <array>
#include <cstdint>

struct Padding {
  int top;
  int right;
  int bottom;
  int left;
};

enum class Layout {
  TILED,
  MONOCLE,
};

struct Vector2D {
  int32_t x;
  int32_t y;
};

[[nodiscard]] inline auto Vector2DToArray(const Vector2D &vec) -> std::array<int32_t, 2> {
  return std::array<int32_t, 2>{vec.x, vec.y};
}

#endif
