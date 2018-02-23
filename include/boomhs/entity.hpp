#pragma once
#include <stlw/type_macros.hpp>
#include <entt/entt.hpp>

namespace boomhs
{

//using EntityID = uint32_t;
static auto constexpr EntityIDMAX = UINT32_MAX;

class EntityRegistry
{
  entt::DefaultRegistry registry_;
public:
  EntityRegistry() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRegistry);

  template<typename Component, typename... Args>
  Component&
  assign(uint32_t const eid, Args&&... args)
  {
    return registry_.assign<Component>(eid, std::forward<Args>(args)...);
  }

  uint32_t create();
  void destroy(uint32_t);

  template<typename T>
  T&
  get(uint32_t const eid)
  {
    assert(has<T>(eid));
    return registry_.get<T>(eid);
  }

  template<typename T>
  T const&
  get(uint32_t const eid) const
  {
    assert(has<T>(eid));
    return registry_.get<T>(eid);
  }

  template<typename T>
  bool
  has() const
  {
    return registry_.has<T>();
  }

  template<typename T>
  bool
  has(uint32_t const eid) const
  {
    assert(eid != EntityIDMAX);
    return registry_.has<T>(eid);
  }

  template<typename ...Args>
  auto
  view()
  {
    return registry_.view<Args...>();
  }
};

} // ns boomhs
