#pragma once
#include <stlw/result.hpp>

namespace engine
{
namespace window
{

// Genric template we expect a library to provide an implementation of.
template <typename L>
struct library_wrapper {
  library_wrapper() = delete;

  static decltype(auto) init() { return L::init(); }

  inline static void destroy() { L::destroy(); }

  inline static decltype(auto) make_window(int const h, int const w)
  {
    return L::make_window(h, w);
  }
};

struct dimensions {
  int const w;
  int const h;
  dimensions(int const wp, int const hp)
      : w(wp)
      , h(hp)
  {
  }
};

} // ns window
} // engine
