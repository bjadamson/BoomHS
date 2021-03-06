#pragma once
#include <common/try.hpp>
#include <common/type_macros.hpp>
#include <extlibs/oktal.hpp>

//
// They each have separate use cases, and build upon the general purpose macros inside
// common/try.hpp.

// DO_EFFECT
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields an error, causes that error to be returned (at the
//   callsite the macro is invoked at).
//
//   * Otherwise immediatly disregards the result (running the values destructor immediatly, if
//   applicable).
//
// HELPER MACRO
#define DO_EFFECT_EVAL(VAR_NAME, expr)                                                             \
  EVAL_INTO_VAR_OR(auto VAR_NAME, expr, [&](auto&& r) { return Err(r.unwrapErrMove()); })

//
// HELPER MACRO
#define DO_EFFECT_CONCAT(VAR_NAME, expr) DO_EFFECT_EVAL(_DO_EFFECT_TEMPORARY_##VAR_NAME, expr)

//
// HELPER MACRO
#define DO_EFFECT_EXPAND_VAR(VAR_NAME, expr) DO_EFFECT_CONCAT(VAR_NAME, expr)

//
// DO_EFFECT MACRO
#define DO_EFFECT(expr) DO_EFFECT_EXPAND_VAR(__COUNTER__, expr)


// OLD_TRY_OR
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields an error, invokes the user provided function on the
//   error, returning the value from the function call.
//
//   The value returned from the function invocation is returned at the callsite of the
//   macro.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define OLD_TRY_OR(VAR_NAME, expr, fn)                                                             \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, [&](auto&& r) { return fn(r.unwrapErr()); })

// OLD_TRY_OR_DEFAULT_T
//
// Evaluates the expression, behaves accordingly.
//   * If evaluating the expression yields an error, return a default instance of T.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define OLD_TRY_OR_DEFAULT_T(VAR_NAME, expr, T)                                                    \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, [](auto const&) { return T{}; })

// OK_MOVE
//
// Shortcut for common idiom:
//    auto st = ...;
//    return Ok(MOVE(st));
//
// becomes
//
//    auto st = ...;
//    return OK_MOVE(st);
#define OK_MOVE(...) Ok(MOVE(__VA_ARGS__))


// TRY_OR
//
// A syntatically nicer version of the TRY_OR macro above.
//
// Performs the same functionality.
//
// NOTE: I am leaving TRY_OR() in this file in case there is a compiler (MSVC? not sure at the time
// of writing this) that will not support this macro invocation.
//
// Uses a compiler extension supported by GCC and clang.
#define TRY_OR(expr, fn)                                                                    \
    ({                                                                                             \
        auto res = expr;                                                                           \
        if (!res.isOk()) {                                                                         \
          typedef details::ResultErrType<decltype(res)>::type E;                                   \
          return fn(res.storage().get<E>());                                                       \
        }                                                                                          \
        typedef details::ResultOkType<decltype(res)>::type T;                                      \
        res.storage().get_moveout<T>();                                                            \
    })

//
// A dummy structure.
namespace common
{

struct Nothing
{
};

using none_t = Nothing;

} // namespace common

#define OK_NONE Ok(common::Nothing{})
#define ERR_NONE Err(common::Nothing{})
