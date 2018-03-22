#pragma once
#include <optional>
#include <stlw/try.hpp>

// MAKEOPT
//
// Evaluates the expression, behaves accordingly:
//   * If evaluating the expression yields none, returns none at the macro invocation site.
//   * Otherwise MOVES the evaluated expression from the "optional value" to the value provided by
//   the caller.
//

#define MAKEOPT(...)                                                                               \
  ({                                                                                               \
    auto opt = __VA_ARGS__;                                                                        \
    if (!opt)                                                                                      \
    {                                                                                              \
      return std::nullopt;                                                                         \
    }                                                                                              \
    *opt;                                                                                          \
  })
