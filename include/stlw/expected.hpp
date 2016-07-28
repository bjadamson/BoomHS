#pragma once
#include <boost/expected/expected.hpp>

namespace stlw
{
// Macros and helper-macros for the DO_MONAD() macro.
#define ONLY_OK(VAR_DECL, V, expr) \
  auto V{std::move(expr)}; \
  if (! V) { return boost::make_unexpected(V.error()); } \
  VAR_DECL {*std::move(V)};

#define ONLY_OK_HELPER(VAR_DECL, V, expr) \
  ONLY_OK(VAR_DECL, expected_##V, expr)

#define ONLY_IFOK_HELPER(VAR_DECL, to_concat, expr) \
  ONLY_OK_HELPER(VAR_DECL, to_concat, expr)

#define DO_MONAD(VAR_DECL, expr) ONLY_IFOK_HELPER(VAR_DECL, __COUNTER__, expr)

// Macros and helper-macros for the DO_EFFECT() macro.
#define INSERT_AUTO(VAR, expr) DO_MONAD(auto VAR, expr)
#define CONCAT(pre, VAR, expr) INSERT_AUTO(pre##VAR, expr)
#define EXPAND_VAR(VAR, expr) CONCAT(__ignoreme__, VAR, expr)
#define DO_EFFECT(expr) EXPAND_VAR(__COUNTER__, expr)
} // ns stlw
