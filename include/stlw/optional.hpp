#pragma once
#include <stlw/try.hpp>
#include <boost/optional.hpp>

namespace stlw
{

template<typename T>
using optional = boost::optional<T>;

template<typename ...P>
auto
make_opt(P &&... p)
{
  return boost::make_optional(std::forward<P>(p)...);
}

// alias
static auto const& none = boost::none;

} // ns stlw

// MAKEOPT_OR_RETURNEARLY
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields none, returns none at the macro invocation site.
//   * Otherwise MOVES the evaluated expression from the "optional value" to the value provided by
//   the caller.
#define MAKEOPT(VAR_NAME, expr)                                                                    \
  EVAL_INTO_VAR_OR(VAR_NAME, expr,                                                                 \
      [](auto const&) { return stlw::none; })
