#pragma once
#include <utility>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{

struct tuple_tag {};
struct container_tag {};

template<typename U, typename TAG>
struct burrito
{
  U value;
  using TAG_TYPE = TAG;
  using VALUE_TYPE = U;

  MOVE_CONSTRUCTIBLE_ONLY(burrito);

  template<typename T>
  explicit constexpr burrito(T &&t) : value(std::move(t)) {}

  auto constexpr size() const
  {
    if constexpr (std::is_same<TAG_TYPE, tuple_tag>()) {
      return std::tuple_size<U>::value;
    } else {
      return value.size();
    }
  }
};

template<template<class, size_t> typename C, typename T, size_t N>
auto constexpr
make_burrito(C<T, N> const& arr)
{
  auto x = stlw::tuple_from_array(arr);
  return burrito<std::decay_t<decltype(x)>, tuple_tag>{std::move(x)};
}

template<template<class, size_t> typename C, typename T, size_t N>
auto constexpr
make_burrito(C<T, N> &&arr)
{
  auto const a = std::move(arr);
  return make_burrito(a);
}

template<typename C>
auto constexpr
make_burrito(C &&c)
{
  return burrito<std::decay_t<C>, container_tag>{std::move(c)};
}

template<typename ...T>
auto constexpr
make_burrito(std::tuple<T...> &&t)
{
  using U = std::tuple<T...>;
  return burrito<U, tuple_tag>{std::move(t)};
}

template<typename ...T>
auto constexpr
make_burrito(T &&... t)
{
  using U = std::tuple<T...>;
  return burrito<U, tuple_tag>{std::make_tuple(std::forward<T>(t)...)};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// HOF's
namespace hof
{
template<typename U, typename FN>
auto constexpr map(burrito<U, tuple_tag> const& b, FN const& fn)
{
  return stlw::make_burrito(stlw::map_tuple_elements(b.value, fn));
}

namespace detail
{
template<typename T, template<class, class> typename C, typename FN>
auto map_impl(C<T, std::allocator<T>> const& c, FN const& fn)
{
  using FN_RT = typename std::decay_t<decltype(fn(*c.cbegin()))>;

  C<FN_RT, std::allocator<FN_RT>> container;
  for (auto const& it : c) {
    container.emplace_back(fn(it));
  }
  return stlw::make_burrito(std::move(container));
}

} // ns detail

template<typename U, typename FN>
auto map(burrito<U, container_tag> const& b, FN const& fn)
{
  using B = std::decay_t<decltype(b)>;
  using C = typename B::VALUE_TYPE;
  using T = typename C::value_type;
  return detail::map_impl<T>(b.value, fn);
}

template<typename U, typename FN>
void constexpr for_each(burrito<U, tuple_tag> const& b, FN const& fn)
{
  stlw::for_each(b.value, fn);
}

template<typename U, typename FN>
void for_each(burrito<U, container_tag> const& b, FN const& fn)
{
  for (auto const& it : b.value) {
    fn(it);
  }
}

} // ns hof

} // ns stlw
