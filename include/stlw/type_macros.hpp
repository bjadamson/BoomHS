#pragma once
#include <utility>

////////////////////////////////////////////////////////////////////////////////////////////////////
// MISC
#define MOVE(a) std::move(a)
#define FORWARD(a) std::forward<decltype(a)>(a)...
#define DEFAULT_CONSTRUCTIBLE(CLASSNAME) CLASSNAME() = default;

////////////////////////////////////////////////////////////////////////////////////////////////////
// COPY
#define NO_COPY_ASSIGN(CLASSNAME) CLASSNAME& operator=(CLASSNAME const&) = delete;
#define NO_COPY_CONSTRUTIBLE(CLASSNAME) CLASSNAME(CLASSNAME const&) = delete;

#define COPY_CONSTRUCTIBLE(CLASSNAME) CLASSNAME(CLASSNAME const&) = default;
#define COPY_ASSIGNABLE(CLASSNAME) CLASSNAME& operator=(CLASSNAME const&) = default;

#define COPY_DEFAULT(CLASSNAME)                                                                    \
  COPY_CONSTRUCTIBLE(CLASSNAME)                                                                    \
  COPY_ASSIGNABLE(CLASSNAME)

#define COPY_CONSTRUCTIBLE_ONLY(CLASSNAME)                                                         \
  NO_MOVE(CLASSNAME)                                                                               \
  NO_COPY_ASSIGN(CLASSNAME)                                                                        \
  COPY_CONSTRUCTIBLE(CLASSNAME)

#define COPY_ONLY(CLASSNAME)                                                                       \
  NO_MOVE(CLASSNAME)                                                                               \
  COPY_DEFAULT(CLASSNAME)

#define NO_COPY(CLASSNAME)                                                                         \
  NO_COPY_ASSIGN(CLASSNAME)                                                                        \
  NO_COPY_CONSTRUTIBLE(CLASSNAME)

////////////////////////////////////////////////////////////////////////////////////////////////////
// MOVE
#define NO_MOVE_ASSIGN(CLASSNAME)                                                                  \
  CLASSNAME& operator=(CLASSNAME&&) = delete;                                                      \
  CLASSNAME& operator=(CLASSNAME const&&) = delete;

#define NO_MOVE_CONSTRUTIBLE(CLASSNAME)                                                            \
  CLASSNAME(CLASSNAME&&)       = delete;                                                           \
  CLASSNAME(CLASSNAME const&&) = delete;

#define MOVE_CONSTRUCTIBLE(CLASSNAME) CLASSNAME(CLASSNAME&&) = default;
#define MOVE_ASSIGNABLE(CLASSNAME) CLASSNAME& operator=(CLASSNAME&&) = default;

#define MOVE_DEFAULT(CLASSNAME)                                                                    \
  MOVE_CONSTRUCTIBLE(CLASSNAME)                                                                    \
  MOVE_ASSIGNABLE(CLASSNAME)

#define MOVE_CONSTRUCTIBLE_ONLY(CLASSNAME)                                                         \
  NO_COPY(CLASSNAME)                                                                               \
  NO_MOVE_ASSIGN(CLASSNAME)                                                                        \
  MOVE_CONSTRUCTIBLE(CLASSNAME)

#define MOVE_ONLY(CLASSNAME)                                                                       \
  NO_COPY(CLASSNAME)                                                                               \
  MOVE_DEFAULT(CLASSNAME)

#define NO_MOVE(CLASSNAME)                                                                         \
  NO_MOVE_CONSTRUTIBLE(CLASSNAME)                                                                  \
  NO_MOVE_ASSIGN(CLASSNAME)

////////////////////////////////////////////////////////////////////////////////////////////////////
// COPY/MOVE
#define COPYMOVE_DEFAULT(CLASSNAME)                                                                \
  COPY_DEFAULT(CLASSNAME)                                                                          \
  MOVE_DEFAULT(CLASSNAME)

#define NO_COPY_OR_MOVE(CLASSNAME)                                                                 \
  NO_COPY(CLASSNAME)                                                                               \
  NO_MOVE(CLASSNAME)

// alias
#define NO_MOVE_OR_COPY(CLASSNAME) NO_COPY_OR_MOVE(CLASSNAME)

////////////////////////////////////////////////////////////////////////////////////////////////////
// std iterators
#define BEGIN_END_FORWARD_FNS(CONTAINER)                                                           \
  decltype(auto) begin() { return CONTAINER.begin(); }                                             \
  decltype(auto) end() { return CONTAINER.end(); }                                                 \
                                                                                                   \
  decltype(auto) begin() const { return CONTAINER.begin(); }                                       \
  decltype(auto) end() const { return CONTAINER.end(); }                                           \
                                                                                                   \
  decltype(auto) cbegin() const { return CONTAINER.cbegin(); }                                     \
  decltype(auto) cend() const { return CONTAINER.cend(); }

#define INDEX_OPERATOR_FNS(CONTAINER)                                                              \
  auto const& operator[](size_t const i) const                                                     \
  {                                                                                                \
    assert(i < CONTAINER.size());                                                                  \
    return CONTAINER[i];                                                                           \
  }                                                                                                \
  auto& operator[](size_t const i)                                                                 \
  {                                                                                                \
    assert(i < CONTAINER.size());                                                                  \
    return CONTAINER[i];                                                                           \
  }

#define COMMON_WRAPPING_CONTAINER_FNS(CONTAINER)                                                   \
  INDEX_OPERATOR_FNS(CONTAINER)                                                                    \
  BEGIN_END_FORWARD_FNS(CONTAINER)                                                                 \
                                                                                                   \
  auto& back()                                                                                     \
  {                                                                                                \
    assert(!CONTAINER.empty());                                                                    \
    return CONTAINER.back();                                                                       \
  }                                                                                                \
  auto const& back() const                                                                         \
  {                                                                                                \
    assert(!CONTAINER.empty());                                                                    \
    return CONTAINER.back();                                                                       \
  }                                                                                                \
  auto& front()                                                                                    \
  {                                                                                                \
    assert(!CONTAINER.empty());                                                                    \
    return CONTAINER.front();                                                                      \
  }                                                                                                \
  auto const& front() const                                                                        \
  {                                                                                                \
    assert(!CONTAINER.empty());                                                                    \
    return CONTAINER.front();                                                                      \
  }                                                                                                \
  auto empty() const { return CONTAINER.empty(); }                                                 \
                                                                                                   \
  auto size() const { return CONTAINER.size(); }

// TODO: document
// BEGIN Function-defining macros
#define DEFINE_WRAPPER_FUNCTION(FN_NAME, FUNCTION_TO_WRAP)                                         \
  template <typename... P>                                                                         \
  decltype(auto) FN_NAME(P&&... p)                                                                 \
  {                                                                                                \
    return FUNCTION_TO_WRAP(FORWARD(p));                                                           \
  }

#define DEFINE_STATIC_WRAPPER_FUNCTION(FN_NAME, FUNCTION_TO_WRAP)                                  \
  template <typename... P>                                                                         \
  static decltype(auto) FN_NAME(P&&... p)                                                          \
  {                                                                                                \
    return FUNCTION_TO_WRAP(FORWARD(p));                                                           \
  }
// END Function-defining macros

namespace stlw
{

namespace impl
{

template <typename T, typename DF>
class ICMW
{
  T  t_;
  DF df_;

  NO_COPY(ICMW)
public:
  ICMW(T const t, DF const df)
      : t_(t)
      , df_(df)
  {
  }

  // Destructor invokes destroy function.
  ~ICMW()
  {
    if (this->df_) {
      this->df_(this->t_);
    }
  }

  // noexcept movable
  ICMW(ICMW&& other)
  noexcept
      : t_(MOVE(other.t_))
      , df_(other.df_)
  {
    other.df_ = nullptr;
  }

  ICMW& operator=(ICMW&& other) noexcept
  {
    this->t_  = MOVE(other.t_);
    this->df_ = other.df_;

    // moved-from doesn't call destroy function
    other.df_ = nullptr;
  }

  // implicitely convertible to underlying type
  operator T() const { return this->t_; }
};

// Wrapper around a function that will be called when the instance of the class
// is destroyed.
//
// Must be passed an r-value, to keep semantics simple.
// Cannot be moved or copied, and this class does NOT expose the function it
// will call after it is
// constructed.
template <typename FN>
class DestroyFN
{
  FN fn_;

public:
  DestroyFN(FN&& fn)
      : fn_(MOVE(fn))
  {
  }
  ~DestroyFN() { this->fn_(); }

  NO_COPY_OR_MOVE(DestroyFN);
};

} // namespace impl

template <typename T, typename DF>
using ImplicitelyCastableMovableWrapper = impl::ICMW<T, DF>;

// Macros and helper-macros for the DO_EFFECT() macro.
#define ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, fn)                                                  \
  ::stlw::impl::DestroyFN<decltype((fn))> const VAR{fn};
#define ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(VAR, expr)                                                \
  auto TEMPORARY##VAR = expr;                                                                      \
  ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, MOVE(TEMPORARY##VAR))

#define ON_SCOPE_EXIT_CONCAT(pre, VAR, expr) ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(pre##VAR, (expr))
#define ON_SCOPE_EXIT_EXPAND(VAR, expr) ON_SCOPE_EXIT_CONCAT(__scopeignoreme__, VAR, expr)
#define ON_SCOPE_EXIT(expr) ON_SCOPE_EXIT_EXPAND(__COUNTER__, expr)

} // namespace stlw
