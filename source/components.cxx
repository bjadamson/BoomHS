#include <boomhs/components.hpp>
#include <boomhs/leveldata.hpp>

namespace boomhs
{

std::vector<uint32_t>
find_stairs_withtype(entt::DefaultRegistry &registry, TileData const& tdata,
    TileType const type)
{
  std::vector<uint32_t> up_stairs;
  auto const visit_fn = [&](auto const& tpos) {
    auto const& tile = tdata.data(tpos);
    if (tile.type == type) {
      up_stairs.emplace_back(tile.eid);
    }
  };
  tdata.visit_each(visit_fn);
  return up_stairs;
}

} // ns boomhs
