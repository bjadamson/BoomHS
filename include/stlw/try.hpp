#pragma once

// These macros are designed to be building blocks for other macros. The intent of the macros these
// macros let you write is to reduce boilerplate in the general case, mostly around dealing with
// optionally returning early from a function.
//
// These macros hide a lot of code of the pattern:
//     if (!expression) {
//       // return early
//     }

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// EVALUATE_OR_RETURN_EARLY
//
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// General macro for evaluating an expression, if that expression yields false this macro invokes
// "returns early" invoking the caller provided function on the contained expression.
#define EVALUATE_OR_RETURN_EARLY(V, expr, or_else_fn)                                              \
  auto V{expr};                                                                                    \
  if (!V) {                                                                                        \
    return or_else_fn(V);                                                                          \
  }

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// EVAL_INTO_VAR_OR_RETURN_EARLY
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// General macro for evaluating some expression, but returning early (for the macro user) if that
// expression yields false.
//
// If the expression evaluates to "true" the expression is moved into the variable provided by the
// macro user.
#define EVAL_INTO_VAR_OR_RETURN_EARLY(VAR_DECL, V, expr, fn)                                       \
  EVALUATE_OR_RETURN_EARLY(V, expr, fn);                                                           \
  VAR_DECL{*MOVE(V)};

// SUPPORT MACRO
#define DO_TRY_CONCAT_EVAL(VAR_DECL, V, expr, fn)                                                  \
  EVAL_INTO_VAR_OR_RETURN_EARLY(VAR_DECL, V, expr, fn)

// SUPPORT MACRO
#define DO_TRY_CONCAT(VAR_DECL, TO_CONCAT, expr, fn)                                               \
  DO_TRY_CONCAT_EVAL(VAR_DECL, _DO_TRY_TEMPORARY_##TO_CONCAT, expr, fn)

// SUPPORT MACRO
#define DO_TRY_EXPAND_VAR(VAR_DECL, to_concat, expr, fn)                                           \
  DO_TRY_CONCAT(VAR_DECL, to_concat, expr, fn)

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// EVAL_INTO_VAR_OR
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// General Purpose Macro
//
// This macro can be used to create more powerful combinators, that all use the variable name macro
// magic to generate unique variable names.
#define EVAL_INTO_VAR_OR(VAR_DECL, expr, fn)                                                              \
  DO_TRY_EXPAND_VAR(VAR_DECL, __COUNTER__, expr, fn)
