#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

struct OrbitalBody
{
  float x_radius = 0.0f;
  float z_radius = 0.0f;

  // Initial orbit offset for the body.
  float offset = 0.0f;
};

struct Sun
{
  float max_height = 0.0f;
  float speed = 0.0f;
};

inline auto
find_suns(EntityRegistry& registry)
{
  std::vector<EntityID> suns;
  auto            const view = registry.view<Sun>();
  for (auto const eid : view)
  {
    assert(registry.has<Transform>(eid));
    suns.emplace_back(eid);
  }
  return suns;
}

} // namespace boomhs
