#pragma once
#include <array>
#include <cassert>
#include <cstring>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>
#include <utility>
#include <vector>

#define FOR(q, n) for (unsigned int q = 0u; q < n; ++q)
#define FORI(q, n) for (int q = 0; q < n; ++q)
#define PAIR(...) std::make_pair(__VA_ARGS__)

namespace stlw::anyof_detail
{

inline bool
orcombo()
{
  return false;
}

template <typename First, typename... Rest>
bool
orcombo(First const& first, Rest&&... rest)
{
  return first || orcombo(FORWARD(rest));
}

} // namespace stlw::anyof_detail

namespace stlw::allof_detail
{

inline bool
allcombo()
{
  return true;
}

template <typename First, typename... Rest>
bool
allcombo(First const& first, Rest&&... rest)
{
  return first && allcombo(FORWARD(rest));
}

} // namespace stlw::allof_detail

#define ALLOF(a, ...) ::stlw::allof_detail::allcombo(a, ##__VA_ARGS__)
#define ANYOF(a, ...) ::stlw::anyof_detail::orcombo(a, ##__VA_ARGS__)

namespace stlw
{

inline void
memzero(void* const dest, size_t const count)
{
  // TODO: move these into a proper test... such a hack
  assert(ANYOF(true, false));
  assert(!ANYOF(false, false));
  assert(ANYOF(false, true));
  assert(!(ANYOF(false)));
  assert(ANYOF(true));

  std::memset(dest, 0, count);
}

template <typename T>
auto
combine_vectors(std::vector<T>&& a, std::vector<T>&& b)
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
template <size_t... Is>
struct seq
{
};
template <size_t N, size_t... Is>

struct gen_seq : gen_seq<N - 1, N - 1, Is...>
{
};

template <size_t... Is>
struct gen_seq<0, Is...> : seq<Is...>
{
};

} // namespace concat_array_algortithm

template <typename T, size_t N1, size_t... I1, size_t N2, size_t... I2>
// Expansion pack
constexpr std::array<T, N1 + N2>
concat(std::array<T, N1> const& a1, std::array<T, N2> const& a2,
       concat_array_algortithm::seq<I1...>, concat_array_algortithm::seq<I2...>)
{
  return {a1[I1]..., a2[I2]...};
}

template <typename T, size_t N1, size_t N2>
// Initializer for the recursion
constexpr std::array<T, N1 + N2>
concat(std::array<T, N1> const& a1, std::array<T, N2> const& a2)
{
  return concat(a1, a2, concat_array_algortithm::gen_seq<N1>{},
                concat_array_algortithm::gen_seq<N2>{});
}

template <typename T, size_t N1>
constexpr std::array<T, N1 + 1>
concat(std::array<T, N1> const& a1, T const& v)
{
  return concat(a1, std::array<T, 1>{v});
}

} // namespace stlw

namespace stlw
{

// source:
// http://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
namespace concat_string_impl
{

template <unsigned...>
struct seq
{
  using type = seq;
};

template <unsigned N, unsigned... Is>
struct gen_seq_x : gen_seq_x<N - 1, N - 1, Is...>
{
};

template <unsigned... Is>
struct gen_seq_x<0, Is...> : seq<Is...>
{
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
length(std::array<T, N> const&)
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
concat_impl(Lhs const& lhs, Rhs const& rhs, seq<I1...>, seq<I2...>)
{
  return {{lhs[I1]..., rhs[I2]..., '\0'}};
}

} // namespace concat_string_impl

template <class Lhs, class Rhs>
constexpr const concat_string_impl::combined_string<Lhs, Rhs>
concat(Lhs const& lhs, Rhs const& rhs)
{
  using namespace concat_string_impl;
  return concat_impl(lhs, rhs, gen_seq<string_length<Lhs>{}>{}, gen_seq<string_length<Rhs>{}>{});
}

template <class T0, class T1, class... Ts>
constexpr const concat_string_impl::combined_string<T0, T1, Ts...>
concat(T0 const& t0, T1 const& t1, Ts const&... ts)
{
  return concat(t0, concat(t1, ts...));
}

template <class T>
constexpr const concat_string_impl::combined_string<T>
concat(T const& t)
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
template <typename FirstBegin, typename FirstEnd, typename SecondBegin, typename FN>
void
zip(FirstBegin fb, FirstEnd fe, SecondBegin sb, FN const& fn)
{
  // Assumes length(sb) > length(fe - fb)
  auto it = sb;
  for (auto i{fb}; i < fe; ++i, ++it)
  {
    fn(*i, *it);
  }
}

template <typename ContainerIter, typename FN, typename... T>
void
zip(FN const& fn, ContainerIter it, std::tuple<T...> const& tuple)
{
  auto const zip_fn = [&fn, &it](auto const& value) {
    fn(value, *it);
    it++;
  };
  stlw::for_each(tuple, zip_fn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// sub_array
template <typename T, size_t N>
auto
sub_array(std::array<T, N> const& data, size_t const begin)
{
  assert(N >= data.size());
  assert(begin <= N);

  std::array<T, N> arr;
  for (auto i{0}; i < N; ++i)
  {
    arr[i] = data[begin + i];
  }
  return arr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// array_from_visitarray
// This function visit each element in the array, invoking a function on each element. This
// function stores the return values of these function invocations in an array that is provided by
// the caller.
template <typename T, typename U, typename FN, size_t N>
void
array_from_visitarray_inplace(std::array<T, N> const& array, FN const& fn, std::array<U, N>* buffer)
{
  FOR(i, N) { (*buffer)[i] = fn(array[i]); }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// array_from_visitarray
// This function visit each element in the array, invoking a function on each element. This
// function stores the return values of these function invocations in an array that is returned to
// the caller.
//
// See "array_from_visitarray_inplace" for modifying an array in place using the same traversal
// pattern.
template <typename T, typename FN, size_t N>
auto
array_from_visitarray(std::array<T, N> const& array, FN const& fn)
{
  using R = decltype(fn(array[0]));
  std::array<R, N> accumulator;
  array_from_visitarray_inplace(array, fn, &accumulator);
  return accumulator;
}

} // namespace stlw
