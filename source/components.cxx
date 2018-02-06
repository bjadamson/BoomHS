#include <boomhs/components.hpp>
#include <boomhs/tilemap.hpp>

namespace boomhs
{

std::vector<uint32_t>
find_stairs_withtype(entt::DefaultRegistry &registry, TileMap const& tmap,
    TileType const type)
{
  std::vector<uint32_t> up_stairs;
  auto const visit_fn = [&](auto const& tpos) {
    auto const& tile = tmap.data(tpos);
    if (tile.type == type) {
      up_stairs.emplace_back(tile.eid);
    }
  };
  tmap.visit_each(visit_fn);
  return up_stairs;
}

} // ns boomhs
