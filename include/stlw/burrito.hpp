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

  MOVE_CONSTRUCTIBLE_ONLY(burrito);

  template<typename T>
  burrito(T &&t) : value(std::move(t)) {}

  template<typename T>
  burrito(T t0, T t1) : value(std::make_pair(t0, t1)) {}

  auto constexpr size() const
  {
    if constexpr (std::is_same<TAG_TYPE, tuple_tag>()) {
      return std::tuple_size<U>::value;
    } else {
      return value.size();
    }
  }
};


// This overload ensures that the types passed in are iterators.
//template<typename T>
//auto constexpr
//make_burrito(T *t0, T *t1)
//{
  //static_assert(std::is_pointer<decltype(t0)>::value, "needs to be pointers");
  //auto const p = std::make_pair(t0, t1);
  //return burrito<decltype(p), container_tag>{p};
//}

/*
template<typename C>
auto constexpr
make_burrito(C &&c)
{
  return make_burrito(c.cbegin(), c.cend());
}
*/

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
template<typename U, typename FN, typename IGNORE=void>
auto constexpr map(burrito<U, tuple_tag> const& b, FN const& fn)
{
  return stlw::make_burrito(stlw::map_tuple_elements(b.value, fn));
}

template<typename U, typename FN, template <class, class> typename Container = std::vector>
auto constexpr map(burrito<U, container_tag> const& b, FN const& fn)
{
  using T = typename std::decay<decltype(*b.value.first)>::type;
  Container<T, std::allocator<T>> container;
  for (auto it{b.value.first}; it < b.value.second; ++it) {
    container.emplace_back(fn(*it));
  }
  return container;
}

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

template<typename U, typename FN>
void constexpr for_each(burrito<U, tuple_tag> const& b, FN const& fn)
{
  stlw::for_each(b.value, fn);
}

template<typename U, typename FN>
void constexpr for_each(burrito<U, container_tag> const& b, FN const& fn)
{
  using T1 = decltype(b.value.first);
  using T2 = decltype(b.value.second);
  static_assert(std::is_same<T1, T2>(), "iterator pairs must be of the same type.");

  for (auto it{b.value.first}; it < b.value.second; ++it) {
    fn(*it);
  }
}

} // ns hof

} // ns stlw
