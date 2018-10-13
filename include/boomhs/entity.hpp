#pragma once
#include <boomhs/transform.hpp>
#include <common/type_macros.hpp>
#include <extlibs/entt.hpp>
#include <extlibs/glm.hpp>

#include <vector>

namespace boomhs
{

using EntityID                    = uint32_t;
static auto constexpr EntityIDMAX = UINT32_MAX;

using EntityArray = std::vector<EntityID>;

class EntityRegistry
{
  entt::DefaultRegistry registry_;

public:
  EntityRegistry() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRegistry);

  template <typename Component, typename... Args>
  Component& assign(EntityID const eid, Args&&... args)
  {
    assert(!has<Component>(eid));
    return registry_.assign<Component>(eid, FORWARD(args));
  }

  EntityID create();
  void     destroy(EntityID);

  template <typename T>
  T& get(EntityID const eid)
  {
    assert(has<T>(eid));
    return registry_.get<T>(eid);
  }

  template <typename T>
  T const& get(EntityID const eid) const
  {
    assert(has<T>(eid));
    return registry_.get<T>(eid);
  }

  template <typename T>
  bool has() const
  {
    return registry_.has<T>();
  }

  template <typename T>
  bool has(EntityID const eid) const
  {
    assert(eid != EntityIDMAX);
    return registry_.has<T>(eid);
  }

  template <typename T>
  void remove(EntityID const eid)
  {
    assert(eid != EntityIDMAX);
    registry_.remove<T>(eid);
  }

  template <typename... Args>
  auto view()
  {
    return registry_.view<Args...>();
  }
};

class EnttLookup
{
  EntityID        eid_ = EntityIDMAX;
  EntityRegistry& registry_;

public:
  explicit EnttLookup(EntityID const eid, EntityRegistry& registry)
      : eid_(eid)
      , registry_(registry)
  {
  }

  template <typename T>
  T& lookup()
  {
    assert(eid_ != EntityIDMAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  template <typename T>
  T const& lookup() const
  {
    assert(eid_ != EntityIDMAX);
    assert(registry_.has<T>(eid_));
    return registry_.get<T>(eid_);
  }

  auto eid() const { return eid_; }

  void set_eid(EntityID const eid) { eid_ = eid; }
};

template <typename... C>
auto
find_all_entities_with_component(EntityRegistry& registry)
{
  EntityArray entities;
  auto const view = registry.view<C...>();
  for (auto const e : view) {
    entities.emplace_back(e);
  }
  return entities;
}

inline auto
all_nearby_entities(glm::vec3 const& pos, float const max_distance, EntityRegistry& registry)
{
  EntityArray entities;
  auto const view = registry.view<Transform>();
  for (auto const e : view) {
    auto& transform = registry.get<Transform>(e);
    if (glm::distance(transform.translation, pos) <= max_distance) {
      entities.emplace_back(e);
    }
  }
  return entities;
}

} // namespace boomhs
