#pragma once

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

  auto size() const { return std::make_pair(right, bottom); }
};

struct ScreenSize
{
  GLsizei const width;
  GLsizei const height;
};

} // namespace boomhs
