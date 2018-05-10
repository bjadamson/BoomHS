#pragma once
#include <stlw/log.hpp>

namespace opengl
{

template <typename T, typename FN>
void
while_bound(stlw::Logger& log, T& obj, FN const& fn)
{
  obj.bind(log);
  ON_SCOPE_EXIT([&]() { obj.unbind(log); });
  fn();
}

} // namespace opengl
