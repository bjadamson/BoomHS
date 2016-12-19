#pragma once
#include <utility>
#include <tuple>

namespace stlw
{

struct tuple_tag {};
struct iterator_tag {};

template<typename TAG, typename U>
struct burrito
{
  U const value;
  using TAG_TYPE = TAG;

  template<typename P>
  explicit burrito(P const& p) : value(U{p}) {}

  template<typename P>
  explicit burrito(P p0, P p1) : value(std::make_pair(p0, p1)) 
  {
    // statically check that P models iterator, and isn't just a pair
    // of U's passed in as a tuple.
    using V = typename P::value_type;
  }
  auto const& unwrap() const { return this->value; }
};

//template<typename B, typename FN>
//void iterate(FN const& fn) const
//{
//if constexpr (std::is_same<tuple_tag, TAG>()) {
//stlw::for_each(this->value, fn);
//}/* else if (std::is_same<iterator_tag, TAG>()) {
//for (auto it{this->value.first}; it < this->value.second; ++it) {
//fn(it);
//}
//}
//}

/*
template<typename T>
auto
make_burrito(T t0, T t1)
{
  auto const p = std::make_pair(t0, t1);
  return burrito<iterator_tag, std::pair<T, T>>{p};
}

template<typename C>
auto
make_burrito(C const& c)
{
  return make_burrito(c.begin(), c.end());
}
*/

template<typename ...T>
auto
make_burrito(std::tuple<T...> const& t)
{
  using U = std::tuple<T...>;
  return burrito<tuple_tag, U>{t};
}

template<typename ...T>
auto
make_burrito(T const&... t)
{
  using U = std::tuple<T...>;
  return burrito<tuple_tag, U>{t...};
}

} // ns stlw
