#pragma once

#define DEFINE_THIS_COPY_DELETED(CLASSNAME) \
  CLASSNAME(CLASSNAME const&) = delete; \
  CLASSNAME& operator=(CLASSNAME const&) = delete;

#define DEFINE_THIS_MOVE_DELETED(CLASSNAME) \
  CLASSNAME(CLASSNAME &&) = delete; \
  CLASSNAME& operator=(CLASSNAME &&) = delete;

#define DEFINE_THIS_MOVE_DEFAULT(CLASSNAME) \
  CLASSNAME(CLASSNAME &&) = default; \
  CLASSNAME& operator=(CLASSNAME &&) = default;

namespace stlw
{

namespace impl
{

template<typename T, typename DF>
class ICMW
{
  T t_;
  DF df_;

  // not-copyable
  DEFINE_THIS_COPY_DELETED(ICMW)
public:
  ICMW(T &&t, DF const df) : t_(std::move(t)), df_(df) {}
  ICMW(T const t, DF const df) : t_(t), df_(df) {}

  // Destructor invokes destroy function.
  ~ICMW()
  {
    if (this->df_) {
      this->df_(this->t_);
    }
  }

  // noexcept movable
  ICMW(ICMW &&other) noexcept :
      t_(std::move(other.t_)),
      df_(other.df_)
  {
    other.df_ = nullptr;
  }

  ICMW& operator=(ICMW &&other) noexcept
  {
    this->t_ = std::move(other.t_);
    this->df_ = other.df_;

    // moved-from doesn't call destroy function
    other.df_ = nullptr;
  }

  // implicitely convertible to underlying type
  operator T() const { return this->t_; }
};

// Wrapper around a function that will be called when the instance of the class is destroyed.
//
// Must be passed an r-value, to keep semantics simple.
// Cannot be moved or copied, and this class does NOT expose the function it will call after it is
// constructed.
template<typename FN>
class DestroyFN
{
  FN fn_;
public:
  DestroyFN(FN &&fn) : fn_(std::forward<FN>(fn)) {}
  ~DestroyFN() { this->fn_(); }

  DEFINE_THIS_COPY_DELETED(DestroyFN)
  DEFINE_THIS_MOVE_DELETED(DestroyFN)
};

} // ns impl

template<typename T, typename DF>
using ImplicitelyCastableMovableWrapper = impl::ICMW<T, DF>;

// Macros and helper-macros for the DO_EFFECT() macro.
#define ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, fn) ::stlw::impl::DestroyFN<decltype((fn))> const VAR{fn};
#define ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(VAR, expr) \
  auto &&TEMPORARY##VAR = std::move(expr); \
  ON_SCOPE_EXIT_CONSTRUCT_IN_PLACE(VAR, std::move(TEMPORARY##VAR))
#define ON_SCOPE_EXIT_CONCAT(pre, VAR, expr) ON_SCOPE_EXIT_MOVE_EXPR_INTO_VAR(pre##VAR, (expr))
#define ON_SCOPE_EXIT_EXPAND(VAR, expr) ON_SCOPE_EXIT_CONCAT(__scopeignoreme__, VAR, expr)
#define ON_SCOPE_EXIT(expr) ON_SCOPE_EXIT_EXPAND(__COUNTER__, expr)

} // ns stlw
