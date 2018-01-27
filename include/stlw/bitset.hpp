#pragma once

// clang-format off
#define DEFINE_OPERATORS_FOR_BITSET_ENUM(ENUM_TYPE)                                                \
using UT = std::underlying_type_t<ENUM_TYPE>;                                                      \
ENUM_TYPE                                                                                          \
operator|(ENUM_TYPE const lhs, ENUM_TYPE const rhs)                                                \
{                                                                                                  \
  return (ENUM_TYPE)(static_cast<T>(lhs) | static_cast<T>(rhs));                                   \
}                                                                                                  \
ENUM_TYPE& operator|=(ENUM_TYPE &lhs, ENUM_TYPE const rhs)                                         \
{                                                                                                  \
    lhs = (ENUM_TYPE)(static_cast<T>(lhs) | static_cast<T>(rhs));                                  \
    return lhs;                                                                                    \
}                                                                                                  \
operator&(ENUM_TYPE const lhs, ENUM_TYPE const rhs)                                                \
{                                                                                                  \
  return (ENUM_TYPE)(static_cast<T>(lhs) & static_cast<T>(rhs));                                   \
}                                                                                                  \
ENUM_TYPE& operator&=(ENUM_TYPE &lhs, ENUM_TYPE const rhs)                                         \
{                                                                                                  \
    lhs = (ENUM_TYPE)(static_cast<T>(lhs) & static_cast<T>(rhs));                                  \
    return lhs;                                                                                    \
}

// clang-format on
