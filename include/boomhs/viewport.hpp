#pragma once
#include <boomhs/color.hpp>
#include <boomhs/math.hpp>
#include <extlibs/fmt.hpp>

#include <string>

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

  // static fns
  static constexpr ScreenSize from_frustum(Frustum const& fr)
  {
    return ScreenSize{fr.width(), fr.height()};
  }

  auto to_string() const
  {
    return fmt::sprintf("{%i, %i}", width, height);
  }
};

using ScreenCoords = glm::ivec2;

class Viewport
{
  int left_, top_, width_, height_;
  Color bg_color_ = LOC::WHITE;

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

  constexpr Viewport(int const left_x, int const top_y, int const width, int const height,
                     Color const& bg_c)
      : Viewport(left_x, top_y, width, height)
  {
    bg_color_ = bg_c;
  }

  constexpr Viewport(glm::ivec2 const& tl, int const width, int const height)
      : Viewport(tl.x, tl.y, width, height)
  {
  }

  constexpr Viewport(std::pair<int, int> const& tl, int const width, int const height)
      : Viewport(tl.first, tl.second, width, height)
  {
  }

  constexpr Viewport(std::pair<int, int> const& tl, int const width, int const height,
                     Color const& bg_c)
      : Viewport(tl.first, tl.second, width, height, bg_c)
  {
  }

  constexpr Viewport(RectInt const& r)
      : Viewport(r.left_top(), r.width(), r.height())
  {
  }

  auto const& bg_color() const { return bg_color_; }
  void set_bg_color(Color const& c) { bg_color_ = c; }

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
  auto constexpr left_bottom() const { return ScreenCoords{left(), bottom()}; }

  auto constexpr right_top() const { return ScreenCoords{right(), top()}; }
  auto constexpr right_bottom() const { return ScreenCoords{right(), bottom()}; }

  auto constexpr rect() const { return RectInt{left(), top(), right(), bottom()}; }
  auto constexpr rect_float() const { return rect().float_rect(); }

  auto constexpr center() const { return ScreenCoords{rect().center()}; }
  auto constexpr center_left() const { return ScreenCoords{rect().center_left()}; }
  auto constexpr center_right() const { return ScreenCoords{rect().center_right()}; }

  auto constexpr center_top() const { return ScreenCoords{rect().center_top()}; }
  auto constexpr center_bottom() const { return ScreenCoords{rect().center_bottom()}; }

  auto constexpr size() const { return ScreenSize{rect().size()}; }
  auto constexpr size_rect() const { return RectInt{left(), top(), width(), height()}; }
  auto constexpr size_rect_float() const { return size_rect().float_rect(); }

  auto to_string() const
  {
    return fmt::sprintf("{%i, %i, %i, %i}", left(), top(), width(), height());
  }

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
