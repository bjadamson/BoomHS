#pragma once
#include <vector>

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

} // ns stlw
