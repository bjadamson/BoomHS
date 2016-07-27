#pragma once

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
  ICMW(ICMW const&) = delete;
  ICMW& operator=(ICMW const&) = delete;
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
template<class FN>
class DestroyFN
{
  FN fn_;
public:
  DestroyFN(FN &&fn) : fn_(std::forward<FN>(fn)) {}
  ~DestroyFN() { this->fn_(); }

  // not-copyable
  DestroyFN(DestroyFN const&) = delete;
  DestroyFN& operator=(DestroyFN const&) = delete;

  // move-constructible, but NOT move-assignable.
  DestroyFN(DestroyFN &&) = default;
  DestroyFN& operator=(DestroyFN &&) = delete;
};

template<class FN>
DestroyFN<FN>
make_destroy_fn(FN &&fn)
{
  return { std::forward<FN>(fn) };
}

} // ns impl

template<typename T, typename DF>
using ImplicitelyCastableMovableWrapper = impl::ICMW<T, DF>;

// Macros and helper-macros for the DO_EFFECT() macro.
#define ON_SCOPE_MAKE(VAR, expr) \
  auto const VAR = ::stlw::impl::make_destroy_fn(expr);

#define SCOPE_CONCAT(pre, VAR, expr) ON_SCOPE_MAKE(pre##VAR, expr)
#define SCOPE_ON_EXIT_EXPAND(VAR, expr) SCOPE_CONCAT(__scopeignoreme__, VAR, expr)
#define ON_SCOPE_EXIT(expr) SCOPE_ON_EXIT_EXPAND(__COUNTER__, expr)

} // ns stlw
