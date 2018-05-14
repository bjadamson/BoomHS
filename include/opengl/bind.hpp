#pragma once
#include <extlibs/fmt.hpp>
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>

// clang-format off
#define DEBUG_BIND(obj)                                                                            \
  FOR_DEBUG_ONLY([&]() { (obj).debug_check.is_bound = true; });

#define DEBUG_UNBIND(obj)                                                                          \
  FOR_DEBUG_ONLY([&]() { (obj).debug_check.is_bound = false; });

#define DEBUG_ASSERT_BOUND(obj)                                                                    \
  FOR_DEBUG_ONLY([&]() { assert((obj).debug_check.is_bound == true); });

#define DEBUG_ASSERT_NOT_BOUND(obj)                                                                \
  FOR_DEBUG_ONLY([&]() { assert((obj).debug_check.is_bound == false); });
// clang-format on

namespace opengl
{

struct DebugBoundCheck
{
#ifdef DEBUG_BUILD
  mutable bool is_bound = false;

  DebugBoundCheck() {}
  NO_COPY(DebugBoundCheck);

  DebugBoundCheck(DebugBoundCheck&& other)
      : is_bound(other.is_bound)
  {
    other.is_bound = false;
  }

  DebugBoundCheck& operator=(DebugBoundCheck&& other)
  {
    is_bound       = other.is_bound;
    other.is_bound = false;
    return *this;
  }
#else
  // This compiles to nothing outside debug builds.
#endif
};

} // namespace opengl

namespace opengl::bind
{

template <typename Obj, typename... Args>
void
global_bind(stlw::Logger& logger, Obj& obj, Args&&... args)
{
  DEBUG_ASSERT_NOT_BOUND(obj);
  obj.bind_impl(logger, FORWARD(args));
  DEBUG_BIND(obj);
}

template <typename Obj, typename... Args>
void
global_unbind(stlw::Logger& logger, Obj& obj, Args&&... args)
{
  DEBUG_ASSERT_BOUND(obj);
  obj.unbind_impl(logger, FORWARD(args));
  DEBUG_UNBIND(obj);
}

} // namespace opengl::bind

#define WHILE_BOUND_IMPL                                                                           \
  {                                                                                                \
    auto const thing_s = fmt::sprintf(obj.to_string());                                            \
                                                                                                   \
    auto const unbind_fn = [&]() {                                                                 \
      LOG_TRACE_SPRINTF("unbind %s", thing_s);                                                     \
      ::opengl::bind::global_unbind(logger, obj);                                                  \
    };                                                                                             \
                                                                                                   \
    LOG_TRACE_SPRINTF("bind %s", thing_s);                                                         \
    ::opengl::bind::global_bind(logger, obj);                                                      \
                                                                                                   \
    ON_SCOPE_EXIT([&]() { unbind_fn(); });                                                         \
    fn();                                                                                          \
  }

namespace opengl::bind
{

template <typename T, typename FN>
void
global_while(stlw::Logger& logger, T& obj, FN const& fn)
{
  WHILE_BOUND_IMPL
}

template <typename R, typename FN>
void
global_while(stlw::Logger& logger, stlw::AutoResource<R>& ar, FN const& fn)
{
  auto& obj = ar.resource();
  WHILE_BOUND_IMPL
}

#undef WHILE_BOUND_IMPL

template <typename T>
void
global_destroy(stlw::Logger& logger, T& obj)
{
  DEBUG_ASSERT_NOT_BOUND(obj);
  obj.destroy();
}

} // namespace opengl::bind

#undef DEBUG_BIND
#undef DEBUG_UNBIND

#define DEFAULT_WHILEBOUND_MEMBERFN_DECLATION()                                                    \
  template <typename FN>                                                                           \
  void while_bound(stlw::Logger& logger, FN const& fn)                                             \
  {                                                                                                \
    ::opengl::bind::global_while(logger, *this, fn);                                               \
  }
