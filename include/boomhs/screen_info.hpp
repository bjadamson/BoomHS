#pragma once
#include <boomhs/math.hpp>

namespace boomhs
{

using PixelT = int;

struct ScreenCoords
{
  PixelT x;
  PixelT y;

  glm::vec2 to_vec2() const { return glm::vec2{x, y}; }
};

struct ScreenSize
{
  PixelT width;
  PixelT height;
};

class ScreenDimensions
{
  PixelT left_, top_, right_, bottom_;

public:
  constexpr ScreenDimensions(PixelT const left, PixelT const top, PixelT const right,
                             PixelT const bottom)
      : left_(left)
      , top_(top)
      , right_(right)
      , bottom_(bottom)
  {
  }

  auto constexpr left() const { return left_; }
  auto constexpr top() const { return top_; }

  auto constexpr right() const { return right_; }
  auto constexpr bottom() const { return bottom_; }

  auto constexpr left_top() const { return ScreenCoords{left(), top()}; }
  auto constexpr right_bottom() const { return ScreenCoords{right(), bottom()}; }

  auto constexpr size() const { return ScreenSize{right(), bottom()}; }
  auto constexpr width() const { return right() - left(); }
  auto constexpr height() const { return bottom() - top(); }

  auto constexpr center() const { return ScreenCoords{width() / 2, height() / 2}; }
  auto constexpr rect() const { return Rectangle{left_, top_, right_, bottom_}; }
};

template <typename T>
constexpr ScreenDimensions
operator/(ScreenDimensions const& n, T const& d)
{
  return ScreenDimensions{
    n.left()   / d,
    n.top()    / d,
    n.right()  / d,
    n.bottom() / d
  };
}

} // namespace boomhs
