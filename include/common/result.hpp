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
#define DO_EFFECT_CONCAT(VAR_NAME, expr)                                                           \
  DO_EFFECT_EVAL(_DO_EFFECT_TEMPORARY_##VAR_NAME, expr)

//
// HELPER MACRO
#define DO_EFFECT_EXPAND_VAR(VAR_NAME, expr)                                                       \
  DO_EFFECT_CONCAT(VAR_NAME, expr)

//
// DO_EFFECT MACRO
#define DO_EFFECT(expr)                                                                            \
  DO_EFFECT_EXPAND_VAR(__COUNTER__, expr)

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

// OLD_TRY_OR_DEFAULT_T
//
// Evaluates the expression, behaves accordingly.
//   * If evaluating the expression yields an error, return a default instance of T.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define OLD_TRY_OR_DEFAULT_T(VAR_NAME, expr, T)                                                    \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, [](auto const&) { return T{}; })

// TRY_OR
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields an error, invokes the user provided function on the
//   error, returning the value from the function call.
//
//   The value returned from the function invocation is returned at the callsite of the
//   macro.
//   * Otherwise MOVES the evaluated expression into the variable VAR_NAME.
#define TRY_OR(VAR_NAME, expr, else_fn)                                                            \
  EVAL_INTO_VAR_OR(VAR_NAME, expr, [&](auto&& r) { return else_fn(r.unwrapErr()); })

//
// HELPER MACRO
#define TRY_EFFECT_OR_HELPER(VAR_NAME, expr, else_fn)                                              \
  TRY_OR(auto VAR_NAME, expr, else_fn)

//
// HELPER MACRO
#define TRY_EFFECT_OR_CONCAT(VAR_NAME, expr, else_fn)                                              \
  TRY_EFFECT_OR_HELPER(_TRY_EFFECT_TEMPORARY_##VAR_NAME, expr, else_fn)

//
// HELPER MACRO
#define TRY_EFFECT_OR_EXPAND_VAR(VAR_NAME, expr, else_fn)                                          \
  TRY_EFFECT_OR_CONCAT(VAR_NAME, expr, else_fn)

// Invoke the expression, if it returns an Error result, return the value of invoking the on_error
// fn.
#define TRY_EFFECT_OR(expr, on_errorfn)                                                  \
  TRY_EFFECT_OR_EXPAND_VAR(__COUNTER__, expr, on_errorfn)

// TRY_OR_COMPOUND_STATEMENT
//
// A syntatically nicer version of the TRY_OR macro above.
//
// Performs the same functionality, but uses a non-standard compiler extension (supported by GCC
// and clang, just not MSVC). Perhaps someday we can switch to this "nicer" implementation,
// allowing cleaner usage at the macro invocatio site.
//
// NOTE: I am leaving TRY_OR() in this file in case there is a compiler (MSVC? not sure at the time
// of writing this) that will not support this macro invocation, due to not recognizing the
// compounte statement compiler extension supported by both gcc/clang.
//
// If Visual Studio ever supports the compound statement extension, or it gets merged into the C++
// standard as a language feature, rename this macro "TRY_OR" and delete the implementation above,
// then rename this macro TRY_OR and update the usage sites to
//
// from:
//   TRY_OR(auto abc, expr, ...)
//
// to:
//   auto abc = TRY_OR(expr, ...);
#define TRY_OR_COMPOUND_STATEMENT(expr, fn)                                                        \
    ({                                                                                             \
        auto res = expr;                                                                           \
        if (!res.isOk()) {                                                                         \
          typedef details::ResultErrType<decltype(res)>::type E;                                   \
          return fn(res.storage().get<E>());                                                       \
        }                                                                                          \
        typedef details::ResultOkType<decltype(res)>::type T;                                      \
        res.storage().get_moveout<T>();                                                            \
    })

namespace common
{

//
// A dummy structure. Used to indicate "Nothing"
struct none_t
{
};
using Nothing = none_t;

} // namespace common

#define OK_NONE Ok(common::Nothing{})
#define ERR_NONE Err(common::Nothing{})
