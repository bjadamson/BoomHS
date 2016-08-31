#pragma once
#include <stlw/result.hpp>

namespace engine
{
namespace gfx
{

// Genric template we expect a library to provide an implementation of.
template <typename L>
struct library_wrapper {
  library_wrapper() = delete;

  static decltype(auto) init() { return L::init(); }

  inline static void destroy() { L::destroy(); }

  template <typename... P>
  inline static decltype(auto) make_gfx_engine(P &&... p)
  {
    return L::make_gfx_engine(std::forward<P>(p)...);
  }
};

} // ns window
} // gfx
