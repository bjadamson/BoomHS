#pragma once
#include <extlibs/spdlog.hpp>

namespace stlw
{
template <typename... Params>
decltype(auto) format(Params &&... p)
{
  return fmt::format(std::forward<Params>(p)...);
}

} // ns stlw
