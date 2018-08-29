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

  auto left() const { return left_; }
  auto top() const { return top_; }

  auto right() const { return right_; }
  auto bottom() const { return bottom_; }

  auto left_top() const { return ScreenCoords{left(), top()}; }
  auto right_bottom() const { return ScreenCoords{right(), bottom()}; }

  auto size() const { return ScreenSize{right(), bottom()}; }
  auto width() const { return right() - left(); }
  auto height() const { return bottom() - top(); }

  auto center() const { return ScreenCoords{width() / 2, height() / 2}; }
  auto as_rectangle() const { return Rectangle{left_, top_, right_, bottom_}; }
};

template <typename T>
ScreenDimensions
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
