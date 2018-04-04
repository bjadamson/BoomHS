#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

namespace boomhs
{

std::vector<EntityID>
find_stairs_withtype(EntityRegistry& registry, TileGrid const& tgrid, TileType const type)
{
  std::vector<EntityID> up_stairs;
  auto const            visit_fn = [&](auto const& tpos) {
    auto const& tile = tgrid.data(tpos);
    if (tile.type == type) {
      up_stairs.emplace_back(tile.eid);
    }
  };
  visit_each(tgrid, visit_fn);
  return up_stairs;
}

} // namespace boomhs
