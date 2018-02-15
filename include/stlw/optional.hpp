#pragma once
#include <stlw/try.hpp>
#include <optional>

// MAKEOPT
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields none, returns none at the macro invocation site.
//   * Otherwise MOVES the evaluated expression from the "optional value" to the value provided by
//   the caller.
#define MAKEOPT(VAR_NAME, expr)                                                                    \
  EVAL_INTO_VAR_OR(VAR_NAME, expr,                                                                 \
      [](auto const&) { return std::nullopt; })
