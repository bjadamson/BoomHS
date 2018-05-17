#pragma once

#include <extlibs/glew.hpp>
#include <utility>

namespace boomhs
{

struct Dimensions
{
  int const x;
  int const y;
  int const w;
  int const h;

  constexpr Dimensions(int const offx, int const offy, int const wp, int const hp)
      : x(offx)
      , y(offy)
      , w(wp)
      , h(hp)
  {
  }

  auto size() const { return std::make_pair(w, h); }
};

struct ScreenSize
{
  GLsizei const width;
  GLsizei const height;
};

} // namespace boomhs
