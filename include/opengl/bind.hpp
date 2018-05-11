#pragma once
#include <extlibs/fmt.hpp>
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#define WHILE_BOUND_IMPL                                                                           \
  {                                                                                                \
    auto const thing_s = fmt::sprintf(obj.to_string());                                            \
                                                                                                   \
    auto const unbind_fn = [&]() {                                                                 \
      LOG_TRACE_SPRINTF("unbind %s", thing_s);                                                     \
      obj.unbind(logger);                                                                          \
    };                                                                                             \
                                                                                                   \
    LOG_TRACE_SPRINTF("binding %s", thing_s);                                                      \
    obj.bind(logger, FORWARD(bind_args));                                                          \
                                                                                                   \
    ON_SCOPE_EXIT([&]() { unbind_fn(); });                                                         \
    fn();                                                                                          \
  }

namespace opengl::bind
{

template <typename T, typename FN, typename... BindArgs>
void
global_while(stlw::Logger& logger, T& obj, FN const& fn, BindArgs&&... bind_args)
{
  WHILE_BOUND_IMPL
}

template <typename R, typename FN, typename... BindArgs>
void
global_while(stlw::Logger& logger, stlw::AutoResource<R>& ar, FN const& fn, BindArgs&&... bind_args)
{
  auto& obj = ar.resource();
  WHILE_BOUND_IMPL
}

} // namespace opengl::bind

#undef WHILE_BOUND_IMPL

#define DEFAULT_WHILEBOUND_MEMBERFN_DECLATION()                                                    \
  template <typename FN, typename... BindArgs>                                                     \
  void while_bound(stlw::Logger& logger, FN const& fn, BindArgs&&... bind_args)                    \
  {                                                                                                \
    ::opengl::bind::global_while(logger, *this, fn, FORWARD(bind_args));                           \
  }
