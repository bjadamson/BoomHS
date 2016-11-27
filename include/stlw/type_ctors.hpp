#pragma once
#include <array>
#include <stlw/types.hpp>
#include <vector>

namespace stlw
{

template <size_t N, typename R, class ...T>
constexpr auto
make_array(T const&... values)
{
  return std::array<R, N>{{ values... }};
}

template <typename T>
auto
vec_with_size(std::size_t const s)
{
  std::vector<T> buffer;
  buffer.resize(s);
  return buffer;
}

// factory FN
constexpr auto
make_empty()
{
  return stlw::empty_type{};
}

} // ns stlw
