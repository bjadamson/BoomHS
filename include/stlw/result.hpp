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

// Macros and helper-macros for the DO_MONAD() macro.
#define DO_MONAD_EVAL(VAR_DECL, V, expr)           \
  auto V{std::move(expr)};                         \
  if (! V) { return stlw::make_error(V.error()); } \
  VAR_DECL {*std::move(V)};

#define DO_MONAD_CONCAT(VAR_DECL, V, expr)             DO_MONAD_EVAL(VAR_DECL, expected_##V, expr)
#define DO_MONAD_EXPAND_VAR(VAR_DECL, to_concat, expr) DO_MONAD_CONCAT(VAR_DECL, to_concat, expr)
#define DO_MONAD(VAR_DECL, expr)                       DO_MONAD_EXPAND_VAR(VAR_DECL, __COUNTER__, expr)

// Macros and helper-macros for the DO_EFFECT() macro.
#define DO_EFFECT_INSERT_AUTO(VAR, expr) DO_MONAD(auto VAR, expr)
#define DO_EFFECT_CONCAT(pre, VAR, expr) DO_EFFECT_INSERT_AUTO(pre##VAR, expr)
#define DO_EFFECT_EXPAND_VAR(VAR, expr)  DO_EFFECT_CONCAT(__ignoreme__, VAR, expr)
#define DO_EFFECT(expr)                  DO_EFFECT_EXPAND_VAR(__COUNTER__, expr)
