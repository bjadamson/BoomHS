#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>
#include <string>

namespace boomhs
{

struct OrbitalBody
{
  std::string name = "Unnamed";
  float       x_radius = 0.0f;
  float       y_radius = 0.0f;
  float       z_radius = 0.0f;

  // Initial orbit offset for the body.
  float offset = 0.0f;
};

inline auto
find_orbital_bodies(EntityRegistry& registry)
{
  std::vector<EntityID> bodies;
  auto const            view = registry.view<OrbitalBody>();
  for (auto const eid : view)
  {
    assert(registry.has<Transform>(eid));
    bodies.emplace_back(eid);
  }
  return bodies;
}

} // namespace boomhs
