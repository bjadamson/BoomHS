#pragma once
#include <boomhs/math.hpp>

namespace boomhs
{

struct ScreenCoords
{
  int x;
  int y;

  glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
};

struct ScreenSize
{
  int width;
  int height;
};

class Viewport
{
  int left_, top_, right_, bottom_;

public:
  constexpr Viewport(int const left, int const top, int const right,
                              int const bottom)
      : left_(left)
      , top_(top)
      , right_(right)
      , bottom_(bottom)
  {
  }

  constexpr Viewport(IntPoint const& tl, IntPoint const& br)
      : Viewport(tl.x, tl.y, br.x, br.y)
  {
  }

  constexpr Viewport(IntRect const& r)
      : Viewport(r.left_top(), r.right_bottom())
  {
  }

  auto constexpr left() const { return left_; }
  auto constexpr top() const { return top_; }

  auto constexpr right() const { return right_; }
  auto constexpr bottom() const { return bottom_; }

  auto constexpr float_left() const { return static_cast<float>(left()); }
  auto constexpr float_top() const { return static_cast<float>(top()); }

  auto constexpr float_right() const { return static_cast<float>(right()); }
  auto constexpr float_bottom() const { return static_cast<float>(bottom()); }

  auto constexpr left_top() const { return ScreenCoords{left(), top()}; }
  auto constexpr right_bottom() const { return ScreenCoords{right(), bottom()}; }

  auto constexpr size() const { return ScreenSize{right(), bottom()}; }
  auto constexpr width() const { return right() - left(); }
  auto constexpr height() const { return bottom() - top(); }

  auto constexpr center() const { return ScreenCoords{width() / 2, height() / 2}; }
  auto constexpr rect() const { return FloatRect{left_, top_, right_, bottom_}; }

  static Viewport
  from_float_rect(FloatRect const& rect)
  {
    return Viewport{
      (int)rect.left,
      (int)rect.top,
      (int)rect.right,
      (int)rect.bottom};
  }
};

template <typename T>
constexpr Viewport
operator/(Viewport const& n, T const& d)
{
  return Viewport{n.left() / d, n.top() / d, n.right() / d, n.bottom() / d};
}

} // namespace boomhs
