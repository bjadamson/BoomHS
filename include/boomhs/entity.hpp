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

class EntityArray
{
  std::vector<EntityID> data_;
public:

  DEFINE_VECTOR_LIKE_WRAPPER_FNS(data_);
};

template <typename ...C>
class EntitySearchResults : public EntityArray
{
  EntityRegistry &registry_;

  // Helper function to iterate over all components in the SearchResults, and help invoke some
  // action for each list of Components (C).
  //
  template <typename USER_FN, typename ACTION>
  void
  iterate_components_with_user_fn(USER_FN const& fn, ACTION const& action) const
  {
    auto const iter_components = [&](auto const eid, auto&&... cs)
    {
      // Forward the variadic list of Components to the user's function.
      //
      // If the user's function returns true, invoke the action function.
      if (fn(FORWARD(cs))) {
        action();
      }
    };

    // Invoke the caller's function on each component-set contained in the EntitySearchResults.
    registry_.view<C...>().each(iter_components);
  }

public:
  explicit EntitySearchResults(EntityRegistry& registry)
      : registry_(registry) { }

  // Count the number of Entities in the SearchResults that match the user's fn.
  template<typename F>
  auto count(F const& fn) const
  {
    int accumulator = 0;
    auto const bump_accum = [&accumulator]() { ++accumulator; };

    iterate_components_with_user_fn(fn, bump_accum);
    return accumulator;
  }

  // Create a new collection of search results (from this) where entities are present
  // if-and-only-if the Entity's component list returns true when passed the user FN.
  template<typename F>
  auto filter(F const& fn) const
  {
    EntitySearchResults<C...> result{registry_};
    auto const add_eid = [&result](auto const eid, auto&&...) { result.emplace_back(eid); };

    iterate_components_with_user_fn(fn, add_eid);
    return result;
  }
};

template <typename... C>
auto
find_all_entities_with_component(EntityRegistry& registry)
{
  EntitySearchResults<C...> result{registry};
  auto const view = registry.view<C...>();
  for (auto const e : view) {
    result.emplace_back(e);
  }
  return result;
}

inline auto
all_nearby_entities(glm::vec3 const& pos, float const max_distance, EntityRegistry& registry)
{
  using C = Transform;
  EntitySearchResults<C> result{registry};

  auto const view = registry.view<C>();
  for (auto const e : view) {
    auto& transform = registry.get<C>(e);
    if (glm::distance(transform.translation, pos) <= max_distance) {
      result.emplace_back(e);
    }
  }
  return result;
}

} // namespace boomhs
