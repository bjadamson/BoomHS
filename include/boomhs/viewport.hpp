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
  int left_, top_, width_, height_;

public:
  constexpr Viewport(int const left_x, int const top_y, int const width, int const height)
      : left_(left_x)
      , top_(top_y)
      , width_(width)
      , height_(height)
  {
    assert(width_ >= 0);
    assert(height_ >= 0);
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

  auto constexpr width() const { return width_; }
  auto constexpr height() const { return height_; }

  auto constexpr half_height() const { return height() / 2; }
  auto constexpr half_width() const { return width() / 2; }

  auto constexpr left() const { return left_; }
  auto constexpr top() const { return top_; }

  auto constexpr right() const { return left() + width(); }
  auto constexpr bottom() const { return top() + height(); }

  auto constexpr float_left() const { return static_cast<float>(left()); }
  auto constexpr float_top() const { return static_cast<float>(top()); }

  auto constexpr float_right() const { return static_cast<float>(right()); }
  auto constexpr float_bottom() const { return static_cast<float>(bottom()); }

  auto constexpr left_top() const { return ScreenCoords{left(), top()}; }
  auto constexpr right_bottom() const { return ScreenCoords{right(), bottom()}; }

  auto constexpr center() const { return ScreenCoords{half_width(), half_height()}; }
  auto constexpr rect() const { return RectInt{left(), top(), right(), bottom()}; }
  auto constexpr rect_float() const { return rect().float_rect(); }

  auto constexpr size() const { return ScreenSize{width(), height()}; }
  auto constexpr size_rect() const { return RectInt{left(), top(), width(), height()}; }
  auto constexpr size_rect_float() const { return size_rect().float_rect(); }

  // static fns
  static constexpr Viewport from_frustum(Frustum const& f)
  {
    // Converting the Frustum to a Viewport discards the NEAR and FAR values, while truncating the
    // following values from floats to integers.
    return Viewport{f.left, f.top, f.width(), f.height()};
  }
};

template <typename T>
constexpr Viewport
operator/(Viewport const& vp, T const& d)
{
  return Viewport{vp.rect() / d};
}

} // namespace boomhs
