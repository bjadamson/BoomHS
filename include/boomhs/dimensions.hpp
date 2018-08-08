#pragma once
#include <stlw/algorithm.hpp>

#include <extlibs/glew.hpp>
#include <utility>

namespace boomhs
{

struct Dimensions
{
  int const left;
  int const top;
  int const right;
  int const bottom;

  constexpr Dimensions(int const l, int const t, int const r, int const b)
      : left(l)
      , top(t)
      , right(r)
      , bottom(b)
  {
  }

  auto size() const { return PAIR(right, bottom); }
  auto width() const { return right - left; }
  auto height() const { return bottom - top; }

  auto center() const { return PAIR(width() / 2, height() / 2); }
};

struct ScreenSize
{
  GLsizei const width;
  GLsizei const height;
};

} // namespace boomhs
