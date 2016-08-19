#pragma once
#include <extlibs/fmt.hpp>

namespace stlw
{
template <typename... Params>
decltype(auto)
format(Params &&... p)
{
  return fmt::format(std::forward<Params>(p)...);
}

} // ns stlw
