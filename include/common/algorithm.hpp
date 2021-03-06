#pragma once
#include <common/tuple.hpp>
#include <common/type_macros.hpp>

#include <array>
#include <cassert>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#define FOR(q, n) for (unsigned int q = 0u; q < n; ++q)
#define FORI(q, n) for (int q = 0; q < n; ++q)
#define PAIR(...) std::pair{__VA_ARGS__}
#define BREAK_THIS_LOOP_IF(v) if (v) { break; }

namespace common
{

template <typename ...Ts>
bool constexpr and_all(Ts... ts) noexcept { return (... && ts); }

template <typename ...Ts>
bool constexpr or_all(Ts... ts) noexcept { return (... || ts); }

template <typename F, typename... Args>
void constexpr
for_each(F f, Args&&... args) noexcept
{
  (f(std::forward<Args>(args)), ...);
}

template <class Target = void, class... Args>
constexpr auto
concat(Args&&... args) noexcept
{
  auto constexpr fn = [](auto&& first, auto&&... rest) {
    using T =
        std::conditional_t<!std::is_void<Target>::value, Target, std::decay_t<decltype(first)>>;
    return std::array<T, (sizeof...(rest) + 1)>{{decltype(first)(first), decltype(rest)(rest)...}};
  };
  return std::apply(fn, std::tuple_cat(FORWARD(args)));
}

inline bool
cstrcmp(char const* a, char const* b)
{
  return 0 == ::strcmp(a, b);
}

inline void
memzero(void* const dest, size_t const count)
{
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
  return dest;
}

template <typename T, size_t N>
auto
vec_from_array(std::array<T, N> const& array)
{
  return std::vector<T>{array.cbegin(), array.cend()};
}

template <typename T, size_t N>
auto
vec_from_array(std::array<T, N> &&array)
{
  return std::vector<T>{array.cbegin(), array.cend()};
}

} // namespace common

namespace common
{

// Given a reference to a value, and a pair of two values, return a reference to the item in the
// pair that is not the same as the value passed in.
//
// ie:
//
// auto a = 5, b = 10;
//
// assert(other_of(a, PAIR(a, b)) == b);
// assert(other_of(b, PAIR(a, b)) == a);
template <typename T>
constexpr T const&
other_of_two(T const& value, std::pair<T, T> const& pair)
{
  return value == pair.first ? pair.second : pair.first;
}

template <typename T, size_t N, class... Args>
constexpr auto
make_array(Args&&... args)
{
  return std::array<T, N>{{FORWARD(args)}};
}

template <typename T, class... Args>
constexpr auto
make_array(Args&&... args)
{
  auto constexpr N = sizeof...(args);
  return make_array<T, N>(FORWARD(args));
}

// Invoke a function 'F' with variadic argument list 'Args'
//
// Any return value from the function 'F' is discarded.
template <typename F, typename...Args>
void constexpr
invoke_fn(F&& f, Args&&...args)
{
  (static_cast<void>(f(std::forward<Args>(args))), ...);
}

// Construct an array with 'N' elements from a function 'F' using arguments 'Args'.
//
// Construct an array of N elements, each element constructed by passing the function F all of the
// arguments Args.
template <size_t N, typename F, typename...Args>
auto constexpr
make_array_from(F&& fn, Args&&...pack)
{
  using T = decltype(fn(FORWARD(pack)));

  std::array<T, N> arr;
  FOR(i, N) {
    arr[i] = fn(FORWARD(pack));
  }
  return arr;
}

// Construct an array of 'N' elements invoking a function 'F' 'N' times.
//
// The function 'F' should accept a single index parameter 'i'. This function 'F' will forward the
// index value 'i' during the array construction to each 'F' invocation.
//
// The parameter 'i' is the current index of the array being constructed.
// ie: 0, 1, 2, ... up to N-1
template <size_t N, typename F>
auto constexpr
make_array_from_fn_forwarding_index(F&& fn)
{
  using T = decltype(fn(N));
  std::array<T, N> arr;
  FOR(i, N) {
    arr[i] = fn(i);
  }

  return arr;
}

// Construct a std::vector<T> with 'n' size.
template <typename T>
auto
vec_with_size(size_t const n)
{
  std::vector<T> buffer;
  buffer.resize(n);
  return buffer;
}

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
concat_string(Lhs const& lhs, Rhs const& rhs)
{
  using namespace concat_string_impl;
  return concat_impl(lhs, rhs, gen_seq<string_length<Lhs>{}>{}, gen_seq<string_length<Rhs>{}>{});
}

template <class T0, class T1, class... Ts>
constexpr const concat_string_impl::combined_string<T0, T1, Ts...>
concat_string(T0 const& t0, T1 const& t1, Ts const&... ts)
{
  return concat_string(t0, concat_string(t1, ts...));
}

template <class T>
constexpr const concat_string_impl::combined_string<T>
concat_string(T const& t)
{
  return concat_string(t, "");
}

constexpr const concat_string_impl::combined_string<>
concat_string()
{
  return concat_string("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// zip
template <typename FirstBegin, typename FirstEnd, typename OutputBegin, typename FN>
void
zip(FirstBegin fb, FirstEnd fe, OutputBegin ob, FN const& fn)
{
  // Assumes length(ob) > length(fe - fb)
  auto it = ob;
  for (auto i{fb}; i < fe; ++i, ++it) {
    fn(*i, *it);
  }
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
  for (auto i{0}; i < N; ++i) {
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

// https://stackoverflow.com/a/217605/562174
// trim from start (in place)
static inline void
ltrim(std::string& s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Trimming algorithms from
// https://stackoverflow.com/a/217605/562174
////////////////////////////////////////////////////////////////////////////////////////////////////
// trim from end (in place)
static inline void
rtrim(std::string& s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
          s.end());
}

// trim from both ends (in place)
static inline void
trim(std::string& s)
{
  ltrim(s);
  rtrim(s);
}

// trim from start (copying)
static inline std::string
ltrim_copy(std::string s)
{
  ltrim(s);
  return s;
}

// trim from end (copying)
static inline std::string
rtrim_copy(std::string s)
{
  rtrim(s);
  return s;
}

// trim from both ends (copying)
static inline std::string
trim_copy(std::string s)
{
  trim(s);
  return s;
}

// Convert an array to a string.
template <typename T>
std::string
stringify(T const& array)
{
  std::string result = "{";
  FOR(i, array.size())
  {
    if (i > 0) {
      result += ", ";
    }
    auto const& v = array[i];
    result += std::to_string(v);
  }
  return result + "}";
}

} // namespace common
