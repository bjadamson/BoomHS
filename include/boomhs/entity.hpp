#pragma once
#include <stlw/type_macros.hpp>
#include <extlibs/entt.hpp>

namespace boomhs
{

using EntityID = uint32_t;
static auto constexpr EntityIDMAX = UINT32_MAX;

class EntityRegistry
{
  entt::DefaultRegistry registry_;
public:
  EntityRegistry() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRegistry);

  template<typename Component, typename... Args>
  Component&
  assign(EntityID const eid, Args &&... args)
  {
    assert(!has<Component>(eid));
    return registry_.assign<Component>(eid, FORWARD(args));
  }

  template<typename Tag, typename... Args>
  Tag & attach(EntityID const eid, Args &&... args)
  {
    assert(!has<Tag>(eid));
    return registry_.attach<Tag>(eid, FORWARD(args));
  }

  EntityID create();
  void destroy(EntityID);

  template<typename T>
  T&
  get(EntityID const eid)
  {
    assert(has<T>(eid));
    return registry_.get<T>(eid);
  }

  template<typename T>
  T const&
  get(EntityID const eid) const
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
  has(EntityID const eid) const
  {
    assert(eid != EntityIDMAX);
    return registry_.has<T>(eid);
  }

  template<typename T>
  void
  remove(EntityID const eid)
  {
    assert(eid != EntityIDMAX);
    registry_.remove<T>(eid);
  }

  template<typename ...Args>
  auto
  view()
  {
    return registry_.view<Args...>();
  }
};

class EnttLookup
{
  EntityID eid_ = EntityIDMAX;
  EntityRegistry &registry_;
public:
  explicit EnttLookup(EntityID const eid, EntityRegistry &registry)
    : eid_(eid)
    , registry_(registry)
  {
  }

  template<typename T>
  T&
  lookup()
  {
    assert(eid_ != EntityIDMAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  template<typename T>
  T const&
  lookup() const
  {
    assert(eid_ != EntityIDMAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  auto eid() const { return eid_; }

  void
  set_eid(EntityID const eid)
  {
    eid_ = eid;
  }
};

} // ns boomhs
