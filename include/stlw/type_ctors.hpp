#pragma once
#include <array>
#include <stlw/types.hpp>
#include <vector>

namespace stlw
{

template <typename T, size_t N, class ...Args>
constexpr auto
make_array(Args &&... args)
{
  return std::array<T, N>{{ std::forward<Args>(args)... }};
}

template <typename T, class ...Args>
constexpr auto
make_array(Args &&... args)
{
  auto constexpr N = sizeof...(args);
  return make_array<T, N>(std::forward<Args>(args)...);
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
