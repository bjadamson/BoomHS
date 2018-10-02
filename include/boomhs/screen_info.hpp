#pragma once
#include <boomhs/math.hpp>

namespace boomhs
{
struct ScreenSize
{
  int width;
  int height;
};

using ScreenCoords = glm::ivec2;

class Viewport
{
  IntRect rect_;

public:
  constexpr Viewport(int const left, int const top, int const right, int const bottom)
      : rect_(left, top, right, bottom)
  {
  }

  constexpr Viewport(glm::ivec2 const& tl, glm::ivec2 const& br)
      : Viewport(tl.x, tl.y, br.x, br.y)
  {
  }

  constexpr Viewport(IntRect const& r)
      : Viewport(r.left_top(), r.right_bottom())
  {
  }

  auto constexpr left() const { return rect_.left; }
  auto constexpr top() const { return rect_.top; }

  auto constexpr right() const { return rect_.right; }
  auto constexpr bottom() const { return rect_.bottom; }

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
  auto constexpr rect() const { return rect_; }
  auto constexpr rect_float() const { return rect().into_float_rect(); }
};

template <typename T>
constexpr Viewport
operator/(Viewport const& n, T const& d)
{
  return Viewport{n.left() / d, n.top() / d, n.right() / d, n.bottom() / d};
}

} // namespace boomhs
