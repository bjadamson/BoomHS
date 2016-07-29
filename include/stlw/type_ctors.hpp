#pragma once
#include <vector>
#include <stlw/types.hpp>

namespace stlw
{

template<typename T>
std::vector<T>
vec_with_size(std::size_t const s)
{
  std::vector<T> buffer;
  buffer.resize(s);
  return buffer;
}

// factory FN
inline stlw::empty_type
make_empty() { return stlw::empty_type{}; }

} // ns stlw
