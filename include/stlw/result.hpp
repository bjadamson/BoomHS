#pragma once
#include <extlibs/expected.hpp>
#include <boost/optional.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/types.hpp>
#include <stlw/try.hpp>

namespace stlw
{

template <typename ...P>
auto
make_error(P &&... p)
{
  return ::nonstd::make_unexpected(std::forward<P>(p)...);
}

template <typename R>
auto
lift_error(R const& result)
{
  return stlw::make_error(result.error());
}

template <typename T, typename E>
using result = ::nonstd::expected<T, E>;
} // ns stlw

// These macros are specific for working with stlw::result<T>.
//
// They each have separate use cases, and build upon the general purpose macros inside
// stlw/try.hpp.

// DO_EFFECT
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields an error, causes that error to be returned (at the
//   callsite the macro is invoked at).
//
//   * If evaluating the expression yields an ok() result, disregards the result (running the
//   values destructor immediatly, if applicable).
#define DO_EFFECT(expr)                                                                            \
  EVAL_INTO_VAR_OR(auto _, expr, stlw::lift_error)                                                 \

// DO_TRY
//
// Tries to evaluate an expression, storing the result into a variable provided by the caller.
#define DO_TRY(VAR_NAME, expr)                                                                     \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, stlw::lift_error);                                              \

// DO_TRY_OR_ELSE_RETURN
//
// Evaluates the expression, behaves accordingly:
//   * If evaluated the expression yields an error, invokes the user provided function  on the
//   error. The value returned from the function invocation is returned at the callsite of the
//   macro.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define DO_TRY_OR_ELSE_RETURN(VAR_NAME, expr, fn)                                                  \
  EVAL_INTO_VAR_OR(VAR_NAME, expr,                                                                 \
      [&fn](auto const& r) { return fn(r.error()); })

// DO_TRY_OR_ELSE_RETURN_DEFAULT_T
//
// Evaluates the expression, behaves accordingly.
//   * If the evaluated expression yields an error, return a default instance of T.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define DO_TRY_OR_ELSE_RETURN_T(VAR_NAME, expr, T)                                                 \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, [](auto const&) { return T{}; })                                \
