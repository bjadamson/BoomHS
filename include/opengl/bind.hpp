#pragma once
#include <extlibs/fmt.hpp>
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

// This file contains the macros and classes that allow the application/game code to bind/unbind
// opengl resources easily, and help ensure that the minimal number of opengl calls to bind
// resources occur. The macro's make sure that a opengl resource bound, is unbound before it is
// bound again. This helps ensure optimal performance, as you can be sure no resource is repeatedly
// bound. All resources must be unbound before they can be bound again. In release mode, unbinding
// the resource currently doesn't do anything. In release mode, so far, it's been find to just let
// everything bind itself (since we know from debug builds things are only bound one time) and
// do without the explicit unbinding call. More testing will have to see if this will need to
// change or not.
//
// *** These macro's compile down to nothing (no source code generated) outside of debug builds.
//
//  There are two sets of macros available from here:
//   1) Macro's that allow easy bind/unbind of opengl resources on demand. This application drawing
//   code that needs to bind a VAO, shader program, before drawing a 3d object.
//
//   {
//     auto vao = ...;
//     BIND_UNTIL_END_OF_SCOPE(logger, vao);
//     ...
//   } // the vao is unbound here
//
//   2) Macro's that make it easy to turn a class/structure into something that itself can be bound
//   manually or with using the macros from #1.
//
//   A minimal example:
//
// class Test
//{
//  // The arguments after logger passed to bind_impl/unbind_impl must match or you will get an
//  // compiler error.
//  void bind_impl(stlw::Logger&, boomhs::Terrain const&, WhateverArgs&, YouWant&, ...));
//  void unbind_impl(stlw::Logger&, boomhs::Terrain const&, WhatArgs&, YouWant&, ...);
//  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();
//};
//
// Then you could use it like:
//
// void drawfn(stlw::Logger&, SomeArgs& a, OtherArg b) {
//   Test instance;
//   instance.while_bound(logger, [&]() { ... });
// }
//
// The while_bound() member function is implemented on your behalf by using the
// DEFAULT_WHILEBOUND_MEMBERFN_DECLATION macro in your class definition.
//
// You can also use the macro's such as BIND_UNTIL_END_OF_SCOPE to manage your resource directly,
// along side with implementing the bind_impl/unbind_impl functions yoursef.

// clang-format off
#define DEBUG_BIND(obj)                                                                            \
  FOR_DEBUG_ONLY([&]() { (obj).debug_check.bound_state = ::opengl::BoundState::BOUND; });

#define DEBUG_UNBIND(obj)                                                                          \
  FOR_DEBUG_ONLY([&]() { (obj).debug_check.bound_state = ::opengl::BoundState::NOT_BOUND; });

#define DEBUG_ASSERT_BOUND(obj)                                                                    \
  FOR_DEBUG_ONLY([&]() { assert((obj).debug_check.bound_state != ::opengl::BoundState::NOT_BOUND); });

#define DEBUG_ASSERT_NOT_BOUND(obj)                                                                \
  FOR_DEBUG_ONLY([&]() { assert((obj).debug_check.bound_state != ::opengl::BoundState::BOUND); });
// clang-format on

namespace opengl
{

enum class BoundState
{
  BOUND,
  NOT_BOUND,
  MOVED_FROM
};

struct DebugBoundCheck
{
#ifdef DEBUG_BUILD
  mutable BoundState bound_state = BoundState::NOT_BOUND;

  DebugBoundCheck() {}
  NO_COPY(DebugBoundCheck);

  DebugBoundCheck(DebugBoundCheck&& other)
      : bound_state(other.bound_state)
  {
    other.bound_state = BoundState::MOVED_FROM;
  }

  DebugBoundCheck& operator=(DebugBoundCheck&& other)
  {
    bound_state       = other.bound_state;
    other.bound_state = BoundState::MOVED_FROM;
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
      ::opengl::bind::global_unbind(logger, obj, FORWARD(args));                                   \
    };                                                                                             \
                                                                                                   \
    LOG_TRACE_SPRINTF("bind %s", thing_s);                                                         \
    ::opengl::bind::global_bind(logger, obj, FORWARD(args));                                       \
                                                                                                   \
    ON_SCOPE_EXIT([&]() { unbind_fn(); });                                                         \
    fn();                                                                                          \
  }

namespace opengl::bind
{

template <typename T, typename FN, typename... Args>
void
global_while(stlw::Logger& logger, T& obj, FN const& fn, Args&&... args)
{
  WHILE_BOUND_IMPL
}

template <typename R, typename FN, typename... Args>
void
global_while(stlw::Logger& logger, stlw::AutoResource<R>& ar, FN const& fn, Args&&... args)
{
  auto& obj = ar.resource();
  WHILE_BOUND_IMPL
}

#undef WHILE_BOUND_IMPL

} // namespace opengl::bind

#undef DEBUG_BIND
#undef DEBUG_UNBIND

#define DEFAULT_WHILEBOUND_MEMBERFN_DECLATION()                                                    \
  template <typename FN, typename... Args>                                                         \
  void while_bound(stlw::Logger& logger, FN const& fn, Args&&... args)                             \
  {                                                                                                \
    ::opengl::bind::global_while(logger, *this, fn, FORWARD(args));                                \
  }                                                                                                \
  template <typename FN, typename... Args>                                                         \
  void while_bound(FN const& fn, stlw::Logger& logger, Args&&... args)                             \
  {                                                                                                \
    ::opengl::bind::global_while(logger, *this, fn, FORWARD(args));                                \
  }

#define BIND_UNTIL_END_OF_SCOPE(LOGGER, BINDEE)                                                    \
  ::opengl::bind::global_bind(LOGGER, BINDEE);                                                     \
  ON_SCOPE_EXIT([&]() { ::opengl::bind::global_unbind(LOGGER, BINDEE); })
