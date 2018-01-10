#pragma once
#include <array>
#include <cassert>
#include <vector>
#include <utility>
#include <stlw/tuple.hpp>

#define FOR(q,n) for(auto q=0u;q<n;++q)

namespace stlw
{

template<typename T>
auto
combine_vectors(std::vector<T> &&a, std::vector<T> &&b)
{
  std::vector<T> dest;
  dest.reserve(a.size() + b.size());

  dest.insert(dest.end(), a.cbegin(), a.cend());
  dest.insert(dest.end(), b.cbegin(), b.cend());
  return MOVE(dest);
}

namespace concat_array_algortithm
{

// http://stackoverflow.com/questions/25068481/c11-constexpr-flatten-list-of-stdarray-into-array
//
// I made it more generic than just 'int' for T.
template <std::size_t... Is>
struct seq {
};
template <std::size_t N, std::size_t... Is>

struct gen_seq : gen_seq<N - 1, N - 1, Is...> {
};

template <std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {
};

} // ns concat_array_algortithm

template <typename T, std::size_t N1, std::size_t... I1, std::size_t N2, std::size_t... I2>
// Expansion pack
constexpr std::array<T, N1 + N2>
concat(std::array<T, N1> const &a1, std::array<T, N2> const &a2,
       concat_array_algortithm::seq<I1...>, concat_array_algortithm::seq<I2...>)
{
  return {a1[I1]..., a2[I2]...};
}

template <typename T, std::size_t N1, std::size_t N2>
// Initializer for the recursion
constexpr std::array<T, N1 + N2>
concat(std::array<T, N1> const &a1, std::array<T, N2> const &a2)
{
  return concat(a1, a2, concat_array_algortithm::gen_seq<N1>{},
                concat_array_algortithm::gen_seq<N2>{});
}

template <typename T, std::size_t N1>
constexpr std::array<T, N1 + 1>
concat(std::array<T, N1> const &a1, T const &v)
{
  return concat(a1, std::array<T, 1>{v});
}

} // ns stlw

namespace stlw
{

// source:
// http://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
namespace concat_string_impl
{

template <unsigned...>
struct seq {
  using type = seq;
};

template <unsigned N, unsigned... Is>
struct gen_seq_x : gen_seq_x<N - 1, N - 1, Is...> {
};

template <unsigned... Is>
struct gen_seq_x<0, Is...> : seq<Is...> {
};

template <unsigned N>
using gen_seq = typename gen_seq_x<N>::type;

template <size_t S>
using size = std::integral_constant<size_t, S>;

template <class T, size_t N>
constexpr size<N>
length(T const (&)[N])
{
  return {};
}

template <class T, size_t N>
constexpr size<N>
length(std::array<T, N> const &)
{
  return {};
}

template <class T>
using length_t = decltype(length(std::declval<T>()));

constexpr size_t
string_size()
{
  return 0;
}

template <class... Ts>
constexpr size_t
string_size(size_t i, Ts... ts)
{
  return (i ? i - 1 : 0) + string_size(ts...);
}
template <class... Ts>
using string_length = size<string_size(length_t<Ts>{}...)>;

template <class... Ts>
using combined_string = std::array<char, string_length<Ts...>{} + 1>;

template <class Lhs, class Rhs, unsigned... I1, unsigned... I2>
constexpr const combined_string<Lhs, Rhs>
concat_impl(Lhs const &lhs, Rhs const &rhs, seq<I1...>, seq<I2...>)
{
  return {{lhs[I1]..., rhs[I2]..., '\0'}};
}

} // ns concat_string_impl

template <class Lhs, class Rhs>
constexpr const concat_string_impl::combined_string<Lhs, Rhs>
concat(Lhs const &lhs, Rhs const &rhs)
{
  using namespace concat_string_impl;
  return concat_impl(lhs, rhs, gen_seq<string_length<Lhs>{}>{}, gen_seq<string_length<Rhs>{}>{});
}

template <class T0, class T1, class... Ts>
constexpr const concat_string_impl::combined_string<T0, T1, Ts...>
concat(T0 const &t0, T1 const &t1, Ts const &... ts)
{
  return concat(t0, concat(t1, ts...));
}

template <class T>
constexpr const concat_string_impl::combined_string<T>
concat(T const &t)
{
  return concat(t, "");
}

constexpr const concat_string_impl::combined_string<>
concat()
{
  return concat("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// zip
template<typename FirstBegin, typename FirstEnd, typename SecondBegin, typename FN>
void zip(FirstBegin fb, FirstEnd fe, SecondBegin sb, FN const& fn)
{
  // Assumes length(sb) > length(fe - fb)
  auto it = sb;
  for (auto i{fb}; i < fe; ++i, ++it) {
    fn(*i, *it);
  }
}

template<typename ContainerIter, typename FN, typename ...T>
void zip(FN const& fn, ContainerIter it, std::tuple<T...> const& tuple)
{
  auto const zip_fn = [&fn, &it](auto const& value) {
    fn(value, *it);
    it++;
  };
  stlw::for_each(tuple, zip_fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// sub_array
template<typename T, std::size_t N>
auto sub_array(std::array<T, N> const& data, std::size_t const begin)
{
  assert(N >= data.size());
  assert(begin <= N);

  std::array<T, N> arr;
  for (auto i{0}; i < N; ++i) {
    arr[i] = data[begin + i];
  }
  return arr;
}

} // ns stlw
