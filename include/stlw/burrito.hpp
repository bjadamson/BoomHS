#pragma once
#include <utility>
#include <tuple>
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

/*
template<typename U, typename FN, template <class, class> typename Container>
auto constexpr map(burrito<U, container_tag> const& b, FN const& fn,
    Container<typename std::decay<decltype(*b.value.first)>::type,
    std::allocator<typename std::decay<decltype(*b.value.first)>::type>> &&cont)
{
  using T1 = decltype(b.value.first);
  using T2 = decltype(b.value.second);
  static_assert(std::is_same<T1, T2>(), "iterator pairs must be of the same type.");

  for (auto it{b.value.first}; it < b.value.second; ++it) {
    cont.emplace_back(fn(*it));
  }
  return cont;
}

template<typename U, typename FN, template <class, std::size_t> typename Container, std::size_t N>
auto constexpr map(burrito<U, container_tag> const& b, FN const& fn,
    Container<typename std::decay<decltype(*b.value.first)>::type, N> &&cont)
{
  using T1 = decltype(b.value.first);
  using T2 = decltype(b.value.second);
  static_assert(std::is_same<T1, T2>(), "iterator pairs must be of the same type.");

  auto i{0};
  for (auto it{b.value.first}; it < b.value.second; ++it) {
    cont[i++] = fn(*it);
  }
  return std::move(cont);
}
*/

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
