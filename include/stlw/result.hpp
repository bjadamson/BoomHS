#pragma once
#include <stlw/types.hpp>
#include <extlibs/boost_expected.hpp>

namespace stlw
{

template<typename ...P>
decltype(auto)
make_error(P &&...p)
{
  return boost::make_unexpected(std::forward<P>(p)...);
}

template<typename T, typename E>
using result = ::boost::expected<T, E>;
} // ns stlw

#define LIFT_ERROR(e) stlw::make_error(e.error())

// General-purpose eval function.
#define DO_GENERAL_EVAL(VAR_DECL, V, expr, wrap_error_fn)\
  auto V{std::move(expr)};                             \
  if (! V) {                                           \
    return wrap_error_fn(stlw::make_error(V.error())); \
  }                                                    \
  VAR_DECL {*std::move(V)};

#define DO_GENERAL_EFFECT(V, expr, or_else_fn)         \
  auto V{std::move(expr)};                             \
  if (! V) {                                           \
    return or_else_fn(V.error());                      \
  } \

// DO_MONAD
#define NOOP_LAMBDA [](auto &&e) { return std::move(e); }
#define DO_MONAD_CONCAT(VAR_DECL, V, expr)             DO_GENERAL_EVAL(VAR_DECL, _DO_MONAD_TEMPORARY_##V, expr, NOOP_LAMBDA)
#define DO_MONAD_EXPAND_VAR(VAR_DECL, to_concat, expr) DO_MONAD_CONCAT(VAR_DECL, to_concat, expr)
#define DO_MONAD(VAR_DECL, expr)                       DO_MONAD_EXPAND_VAR(VAR_DECL, __COUNTER__, expr)

// DO_EFFECT
#define DO_EFFECT_INSERT_AUTO(VAR, expr) DO_MONAD(auto VAR, expr)
#define DO_EFFECT_CONCAT(pre, VAR, expr) DO_EFFECT_INSERT_AUTO(pre##VAR, expr)
#define DO_EFFECT_EXPAND_VAR(VAR, expr)  DO_EFFECT_CONCAT(_DO_EFFECT_MACRO_TEMPORARY_, VAR, expr)
#define DO_EFFECT(expr)                  DO_EFFECT_EXPAND_VAR(__COUNTER__, expr)

// OR_ELSE
#define DO_MONAD_OR_ELSE_EVAL(VAR_DECL, V, expr, fn)           DO_GENERAL_EVAL(VAR_DECL, V, expr, fn)
#define DO_MONAD_OR_ELSE_CONCAT(VAR_DECL, to_concat, expr, fn) DO_MONAD_OR_ELSE_EVAL(VAR_DECL, _DO_MONAD_OR_ELSE_TEMPORARY_##to_concat, expr, fn)
#define DO_MONAD_OR_ELSE_EXPAND(VAR_DECL, to_concat, expr, fn) DO_MONAD_OR_ELSE_CONCAT(VAR_DECL, to_concat, expr, fn)
#define DO_MONAD_OR_ELSE(VAR_DECL, expr, fn)                   DO_MONAD_OR_ELSE_EXPAND(VAR_DECL, __COUNTER__, expr, fn)

// DO_EFFECT_OR_ELSE
#define DO_EFFECT_OR_ELSE_GENERAL_EVAL(VAR, expr, fn) DO_GENERAL_EFFECT(VAR, expr, fn)
#define DO_EFFECT_OR_ELSE_CONCAT(pre, VAR, expr, fn) DO_EFFECT_OR_ELSE_GENERAL_EVAL(pre##VAR, expr, fn)
#define DO_EFFECT_OR_ELSE_EXPAND(VAR, expr, fn) DO_EFFECT_OR_ELSE_CONCAT(_DO_EFFECT_OR_ELSE_TEMPORARY_, VAR, expr, fn)
#define DO_EFFECT_OR_ELSE(expr, fn) DO_EFFECT_OR_ELSE_EXPAND(__COUNTER__, expr, fn)
