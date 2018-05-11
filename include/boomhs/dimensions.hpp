#pragma once
#include <extlibs/glew.hpp>

namespace boomhs
{

struct Dimensions
{
  int const x;
  int const y;
  int const w;
  int const h;
  Dimensions(int const offx, int const offy, int const wp, int const hp)
      : x(offx)
      , y(offy)
      , w(wp)
      , h(hp)
  {
  }
};

struct ScreenSize
{
  GLsizei const width;
  GLsizei const height;
};

} // namespace boomhs
