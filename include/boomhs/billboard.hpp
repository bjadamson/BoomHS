#pragma once
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

  static BillboardType
  from_string(std::string const& str)
  {
    if ("spherical" == str) {
      return BillboardType::Spherical;
    }
    else if ("cylindrical" == str) {
      return BillboardType::Cylindrical;
    }
    else {
      std::abort();
    }
  }
};

struct BillboardRenderable
{
  BillboardType value = BillboardType::INVALID;
};

inline auto
find_billboards(EntityRegistry &registry)
{
  std::vector<EntityID> bboards;
  auto view = registry.view<BillboardRenderable>();
  for (auto const eid : view) {
    assert(registry.has<Transform>(eid));
    bboards.emplace_back(eid);
  }
  return bboards;
}

} // ns boomhs
