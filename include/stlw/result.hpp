#pragma once
#include <extlibs/boost_expected.hpp>
#include <stlw/types.hpp>

namespace stlw
{

template <typename... P>
decltype(auto)
make_error(P &&... p)
{
  return boost::make_unexpected(std::forward<P>(p)...);
}

template <typename T, typename E>
using result = ::boost::expected<T, E>;
} // ns stlw

// General-purpose eval function.
#define DO_GENERAL_EVAL(VAR_DECL, V, expr)                                                         \
  auto V{std::move(expr)};                                                                         \
  if (!V) {                                                                                        \
    return stlw::make_error(V.error());                                                            \
  }                                                                                                \
  VAR_DECL{*std::move(V)};

// DO_TRY
#define DO_TRY_CONCAT(VAR_DECL, V, expr) DO_GENERAL_EVAL(VAR_DECL, _DO_TRY_TEMPORARY_##V, expr)
#define DO_TRY_EXPAND_VAR(VAR_DECL, to_concat, expr) DO_TRY_CONCAT(VAR_DECL, to_concat, expr)
#define DO_TRY(VAR_DECL, expr) DO_TRY_EXPAND_VAR(VAR_DECL, __COUNTER__, expr)

// OR_ELSE
#define DO_GENERAL_OR_ELSE_EVAL(VAR_DECL, V, expr, fn)                                             \
  auto V{std::move(expr)};                                                                         \
  if (!V) {                                                                                        \
    fn(V.error());                                                                                 \
  }                                                                                                \
  VAR_DECL{*std::move(V)};

#define DO_TRY_OR_ELSE_EVAL(VAR_DECL, V, expr, fn) DO_GENERAL_OR_ELSE_EVAL(VAR_DECL, V, expr, fn)
#define DO_TRY_OR_ELSE_CONCAT(VAR_DECL, to_concat, expr, fn)                                       \
  DO_TRY_OR_ELSE_EVAL(VAR_DECL, _DO_TRY_OR_ELSE_TEMPORARY_##to_concat, expr, fn)
#define DO_TRY_OR_ELSE_EXPAND(VAR_DECL, to_concat, expr, fn)                                       \
  DO_TRY_OR_ELSE_CONCAT(VAR_DECL, to_concat, expr, fn)
#define DO_TRY_OR_ELSE_RETURN(VAR_DECL, expr, fn)                                                  \
  DO_TRY_OR_ELSE_EXPAND(VAR_DECL, __COUNTER__, expr, fn)

// DO_EFFECT
#define DO_EFFECT_INSERT_AUTO(VAR, expr) DO_TRY(auto VAR, expr)
#define DO_EFFECT_CONCAT(pre, VAR, expr) DO_EFFECT_INSERT_AUTO(pre##VAR, expr)
#define DO_EFFECT_EXPAND_VAR(VAR, expr) DO_EFFECT_CONCAT(_DO_EFFECT_MACRO_TEMPORARY_, VAR, expr)
#define DO_EFFECT(expr) DO_EFFECT_EXPAND_VAR(__COUNTER__, expr)

// DO_EFFECT_OR_ELSE
#define DO_GENERAL_EFFECT(V, expr, or_else_fn)                                                     \
  auto V{std::move(expr)};                                                                         \
  if (!V) {                                                                                        \
    return or_else_fn(V.error());                                                                  \
  }

#define DO_EFFECT_OR_ELSE_GENERAL_EVAL(VAR, expr, fn) DO_GENERAL_EFFECT(VAR, expr, fn)
#define DO_EFFECT_OR_ELSE_CONCAT(pre, VAR, expr, fn)                                               \
  DO_EFFECT_OR_ELSE_GENERAL_EVAL(pre##VAR, expr, fn)
#define DO_EFFECT_OR_ELSE_EXPAND(VAR, expr, fn)                                                    \
  DO_EFFECT_OR_ELSE_CONCAT(_DO_EFFECT_OR_ELSE_TEMPORARY_, VAR, expr, fn)
#define DO_EFFECT_OR_ELSE_RETURN(expr, fn) DO_EFFECT_OR_ELSE_EXPAND(__COUNTER__, expr, fn)
