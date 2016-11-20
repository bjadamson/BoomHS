#pragma once

// BEGIN Class-building macros
#define NO_COPY_ASSIGN(CLASSNAME)                                                                  \
  CLASSNAME &operator=(CLASSNAME const &) = delete;

#define NO_COPY(CLASSNAME)                                                                         \
  CLASSNAME(CLASSNAME const &) = delete;                                                           \
  NO_COPY_ASSIGN(CLASSNAME);

#define NO_MOVE_ASSIGN(CLASSNAME)                                                                  \
  CLASSNAME &operator=(CLASSNAME &&) = delete;

#define NO_MOVE(CLASSNAME)                                                                         \
  CLASSNAME(CLASSNAME &&) = delete;                                                                \
  NO_MOVE_ASSIGN(CLASSNAME);                                                                       \

#define NO_COPY_AND_NO_MOVE(CLASSNAME)                                                             \
  NO_COPY(CLASSNAME)                                                                               \
  NO_MOVE(CLASSNAME)

#define MOVE_DEFAULT(CLASSNAME)                                                                    \
  CLASSNAME(CLASSNAME &&) = default;                                                               \
  CLASSNAME &operator=(CLASSNAME &&) = default;

#define COPY_DEFAULT(CLASSNAME)                                                                    \
  CLASSNAME(CLASSNAME const&) = default;                                                           \
  CLASSNAME &operator=(CLASSNAME const&) = default;

#define MOVE_CONSTRUCTIBLE(CLASSNAME)                                                              \
  CLASSNAME(CLASSNAME &&) = default;
// END class-builing macros

// BEGIN Function-defining macros
#define DEFINE_WRAPPER_FUNCTION(FN_NAME, FUNCTION_TO_WRAP)                                         \
  template <typename... P>                                                                         \
  decltype(auto) FN_NAME(P &&... p)                                                                \
  {                                                                                                \
    return FUNCTION_TO_WRAP(std::forward<P>(p)...);                                                \
  }

#define DEFINE_STATIC_WRAPPER_FUNCTION(FN_NAME, FUNCTION_TO_WRAP)                                  \
  template <typename... P>                                                                         \
  static decltype(auto) FN_NAME(P &&... p)                                                         \
  {                                                                                                \
    return FUNCTION_TO_WRAP(std::forward<P>(p)...);                                                \
  }
// END Function-defining macros

namespace stlw
{

namespace impl
{

template <typename T, typename DF>
class ICMW
{
  T t_;
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
  ICMW(ICMW &&other)
  noexcept
      : t_(std::move(other.t_))
      , df_(other.df_)
  {
    other.df_ = nullptr;
  }

  ICMW &operator=(ICMW &&other) noexcept
  {
    this->t_ = std::move(other.t_);
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
  DestroyFN(FN &&fn)
      : fn_(std::forward<FN>(fn))
  {
  }
  ~DestroyFN() { this->fn_(); }

  NO_COPY_AND_NO_MOVE(DestroyFN);
};

} // ns impl

template <typename T, typename DF>
using ImplicitelyCastableMovableWrapper = impl::ICMW<T, DF>;

// Macros and helper-macros for the DO_EFFECT() macro.
#define ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, fn)                                                  \
  ::stlw::impl::DestroyFN<decltype((fn))> const VAR{fn};
#define ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(VAR, expr)                                                \
  auto &&TEMPORARY##VAR = std::move(expr);                                                         \
  ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, std::move(TEMPORARY##VAR))
#define ON_SCOPE_EXIT_CONCAT(pre, VAR, expr) ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(pre##VAR, (expr))
#define ON_SCOPE_EXIT_EXPAND(VAR, expr) ON_SCOPE_EXIT_CONCAT(__scopeignoreme__, VAR, expr)
#define ON_SCOPE_EXIT(expr) ON_SCOPE_EXIT_EXPAND(__COUNTER__, expr)

} // ns stlw
