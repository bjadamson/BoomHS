#pragma once
#include <stlw/tuple.hpp>

namespace stlw
{

// Checks that only one of the provided boolean values is true.
//
// * If no value are true, this function asserts().
// * If two values (or more) are true, this function asserts.
template<typename ...T>
inline void
assert_exactly_only_one_true(T const&... bools)
{
  bool one_true = false;
  auto const assert_fn = [&one_true](bool const b)
  {
    // If already found a true value, this value must not be true.
    if (one_true) {
      assert(!b);
    }
    one_true |= b;
  };
  auto const tuple = std::make_tuple(bools...);
  stlw::for_each(tuple, assert_fn);
}

} // ns stlw
