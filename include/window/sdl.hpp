#pragma once
#include <extlibs/sdl.hpp>

namespace window
{

struct Dimensions {
  int const w;
  int const h;
  Dimensions(int const wp, int const hp)
      : w(wp)
      , h(hp)
  {
  }
};

} // ns window
