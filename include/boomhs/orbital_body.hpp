#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <extlibs/glm.hpp>
#include <string>

namespace boomhs
{

struct OrbitalBody
{
  float x_radius = 0.0f;
  float y_radius = 0.0f;
  float z_radius = 0.0f;

  // Initial orbit offset for the body.
  float offset = 0.0f;

  static void add_to_entity(EntityID const eid, EntityRegistry& registry)
  {
    glm::vec3 constexpr min = glm::vec3{-1.0f};
    glm::vec3 constexpr max = glm::vec3{1.0f};
    AABoundingBox::add_to_entity(eid, registry, min, max);
  }
};

inline auto
find_orbital_bodies(EntityRegistry& registry)
{
  std::vector<EntityID> bodies;
  auto const            view = registry.view<OrbitalBody>();
  for (auto const eid : view) {
    assert(registry.has<Transform>(eid));
    bodies.emplace_back(eid);
  }
  return bodies;
}

} // namespace boomhs
