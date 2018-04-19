#pragma once
#include <stlw/type_macros.hpp>

#include <array>
#include <vector>

namespace stlw
{

template <typename T, size_t N, class... Args>
constexpr auto
make_array(Args&&... args)
{
  return std::array<T, N>{{FORWARD(args)}};
}

template <typename T, class... Args>
constexpr auto
make_array(Args&&... args)
{
  auto constexpr N = sizeof...(args);
  return make_array<T, N>(FORWARD(args));
}

template <typename T>
auto
vec_with_size(size_t const s)
{
  std::vector<T> buffer;
  buffer.resize(s);
  return buffer;
}

} // namespace stlw
