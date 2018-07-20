#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <string>

namespace boomhs
{

enum class BillboardType
{
  Spherical = 0,
  Cylindrical,
  INVALID
};

struct Billboard
{
  Billboard() = delete;

  static BillboardType from_string(std::string const&);

  // Compute the viewmodel for a transform that appears as billboard.
  static glm::mat4
  compute_viewmodel(Transform const&, glm::mat4 const&, BillboardType);
};

struct BillboardRenderable
{
  BillboardType value = BillboardType::INVALID;
};

inline auto
find_billboards(EntityRegistry& registry)
{
  std::vector<EntityID> bboards;
  auto                  view = registry.view<BillboardRenderable>();
  for (auto const eid : view) {
    assert(registry.has<Transform>(eid));
    bboards.emplace_back(eid);
  }
  return bboards;
}

} // namespace boomhs
