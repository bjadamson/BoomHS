#pragma once
#include <extlibs/sdl.hpp>

namespace window
{

struct Dimensions {
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

} // ns window
