#pragma once
#include <boomhs/math.hpp>

namespace boomhs
{
struct ScreenSize
{
  int width;
  int height;

  constexpr ScreenSize(int const w, int const h)
      : width(w)
      , height(h)
  {
  }

  constexpr ScreenSize(glm::ivec2 const& p)
      : ScreenSize(p.x, p.y)
  {
  }
};

using ScreenCoords = glm::ivec2;

class Viewport
{
  RectInt rect_;

public:

  // Conversion constructor, converts user arguments (left, top, right, bottom) to what the
  // viewport expects (left, bottom, width, height)
  constexpr Viewport(int const left_x, int const top_y, int const width, int const height)
      : rect_(left_x, top_y, left_x + width, top_y + height)
  {
  }

  constexpr Viewport(glm::ivec2 const& tl, int const width, int const height)
      : Viewport(tl.x, tl.y, width, height)
  {
  }

  constexpr Viewport(std::pair<int, int> const& tl, int const width, int const height)
      : Viewport(tl.first, tl.second, width, height)
  {
  }

  constexpr Viewport(RectInt const& r)
      : Viewport(r.left_top(), r.width(), r.height())
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

  auto constexpr size() const { return ScreenSize{rect_.size()}; }
  auto width() const { return rect_.width(); }
  auto height() const { return rect_.height(); }

  auto half_height() const { return height() / 2; }
  auto half_width() const { return width() / 2; }

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
