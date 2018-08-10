#pragma once
#include <experimental/tuple>
#include <functional>
#include <tuple>
#include <type_traits>

// This code from stack overflow. The current implementation of std::experimental:P:apply doesn't
// work for the use cases needed, so we use this implementation instead.
//
// Someday replace this with std::apply, once it is finished implemented.
// http://codereview.stackexchange.com/questions/51407/stdtuple-foreach-implementation

namespace common
{

template <class T>
constexpr bool is_function_v = std::is_function<T>::value;

template <class Base, class Derived>
constexpr bool is_base_of_v = std::is_base_of<Base, Derived>::value;

template <class T>
constexpr bool is_member_pointer_v = std::is_member_pointer<T>::value;

template <class T>
constexpr size_t tuple_size_v = std::tuple_size<T>::value;

template <typename Tuple, typename F, size_t... Indices>
void
for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
{
  using swallow = int[];
  (void)swallow{1, (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...};
}

template <typename Tuple, typename F>
void
for_each(Tuple&& tuple, F&& f)
{
  constexpr size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
  for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f), std::make_index_sequence<N>{});
}

/////
// invoke
namespace detail
{
template <class T>
struct is_reference_wrapper : std::false_type
{
};
template <class U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type
{
};
template <class T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

template <class Base, class T, class Derived, class... Args>
auto
INVOKE(T Base::*pmf, Derived&& ref, Args&&... args) noexcept(
    noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...)))
    -> std::enable_if_t<is_function_v<T> && is_base_of_v<Base, std::decay_t<Derived>>,
                        decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))>
{
  return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class RefWrap, class... Args>
auto
INVOKE(T Base::*pmf, RefWrap&& ref,
       Args&&... args) noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
    -> std::enable_if_t<is_function_v<T> && is_reference_wrapper_v<std::decay_t<RefWrap>>,
                        decltype((ref.get().*pmf)(std::forward<Args>(args)...))>

{
  return (ref.get().*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class Pointer, class... Args>
auto
INVOKE(T Base::*pmf, Pointer&& ptr, Args&&... args) noexcept(
    noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
    -> std::enable_if_t<is_function_v<T> && !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                            !is_base_of_v<Base, std::decay_t<Pointer>>,
                        decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))>
{
  return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
}

template <class Base, class T, class Derived>
auto
INVOKE(T Base::*pmd, Derived&& ref) noexcept(noexcept(std::forward<Derived>(ref).*pmd))
    -> std::enable_if_t<!is_function_v<T> && is_base_of_v<Base, std::decay_t<Derived>>,
                        decltype(std::forward<Derived>(ref).*pmd)>
{
  return std::forward<Derived>(ref).*pmd;
}

template <class Base, class T, class RefWrap>
auto
INVOKE(T Base::*pmd, RefWrap&& ref) noexcept(noexcept(ref.get().*pmd))
    -> std::enable_if_t<!is_function_v<T> && is_reference_wrapper_v<std::decay_t<RefWrap>>,
                        decltype(ref.get().*pmd)>
{
  return ref.get().*pmd;
}

template <class Base, class T, class Pointer>
auto
INVOKE(T Base::*pmd, Pointer&& ptr) noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
    -> std::enable_if_t<!is_function_v<T> && !is_reference_wrapper_v<std::decay_t<Pointer>> &&
                            !is_base_of_v<Base, std::decay_t<Pointer>>,
                        decltype((*std::forward<Pointer>(ptr)).*pmd)>
{
  return (*std::forward<Pointer>(ptr)).*pmd;
}

template <class F, class... Args>
auto
INVOKE(F&& f, Args&&... args) noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
    -> std::enable_if_t<!is_member_pointer_v<std::decay_t<F>>,
                        decltype(std::forward<F>(f)(std::forward<Args>(args)...))>
{
  return std::forward<F>(f)(std::forward<Args>(args)...);
}
} // namespace detail

template <class F, class... ArgTypes>
auto
invoke(F&& f, ArgTypes&&... args)
    // exception specification for QoI
    noexcept(noexcept(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...)))
        -> decltype(detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...))
{
  return detail::INVOKE(std::forward<F>(f), std::forward<ArgTypes>(args)...);
}

/////
// apply
template <typename F, typename Tuple, size_t... S>
decltype(auto)
apply_tuple_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
  return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
decltype(auto)
apply(F&& fn, Tuple&& t)
{
  size_t constexpr tSize = std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
  return apply_tuple_impl(std::forward<F>(fn), std::forward<Tuple>(t),
                          std::make_index_sequence<tSize>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// to_tuple
namespace detail
{
template <typename C, size_t... Indices>
auto
to_tuple_helper(C const& c, std::index_sequence<Indices...>)
{
  return std::make_tuple(c[Indices]...);
}

} // namespace detail

template <size_t N, typename C>
auto
to_tuple(C const& v)
{
  assert(v.size() >= N);
  return detail::to_tuple_helper(v, std::make_index_sequence<N>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// map_tuple_elements
namespace detail
{
template <typename T, typename F, size_t... Is>
auto constexpr map_tuple_elements(T&& tup, F& f, std::index_sequence<Is...>)
{
  return std::make_tuple(f(std::get<Is>(std::forward<T>(tup)))...);
}

} // namespace detail

template <typename T, typename F, size_t TupSize = common::tuple_size_v<std::decay_t<T>>>
auto constexpr map_tuple_elements(T&& tup, F f)
{
  return detail::map_tuple_elements(std::forward<T>(tup), f, std::make_index_sequence<TupSize>{});
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// tuple_from_array
namespace detail
{
template <typename T, size_t... Is>
auto constexpr tuple_from_array(T const& arr, std::index_sequence<Is...>)
{
  return std::make_tuple(arr[Is]...);
}

template <size_t N, typename V, typename T, size_t... Is>
auto constexpr array_from_container(T const& c, std::index_sequence<Is...>)
{
  return std::array<V, N>{c[Is]...};
}

} // namespace detail

template <typename T>
auto constexpr tuple_from_array(T const& arr)
{
  auto constexpr tup_size = std::tuple_size<std::decay_t<T>>::value;
  return detail::tuple_from_array(arr, std::make_index_sequence<tup_size>{});
}

template <typename T, size_t N>
auto constexpr tuple_from_array(T const (&arr)[N])
{
  return detail::tuple_from_array(arr, std::make_index_sequence<N>{});
}

// not safe
template <size_t N, typename T>
auto constexpr tuple_from_container(T const& c)
{
  using V = typename T::value_type;
  return tuple_from_array(detail::array_from_container<N, V>(c, std::make_index_sequence<N>{}));
}

} // namespace common
