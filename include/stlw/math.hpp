#pragma once
#include <array>
#include <utility>

namespace stlw::math
{

// Normalizes "value" from the "from_range" to the "to_range"
template <typename T, typename P1, typename P2>
constexpr float
normalize(T const value, P1 const& from_range, P2 const& to_range)
{
  static_assert(std::is_integral<T>::value, "Input must be integral");
  auto const minimum = from_range.first;
  auto const maximum = from_range.second;
  auto const floor = to_range.first;
  auto const ceil = to_range.second;
  auto const normalized = ((ceil - floor) * (value - minimum))/(maximum - minimum) + floor;
  return static_cast<float>(normalized);
}

} // ns math::stlw
