#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

namespace boomhs
{

AxisAlignedBoundingBox::AxisAlignedBoundingBox()
{
  auto constexpr DEFAULT_SIZE = 1.0f;

  min.x = -DEFAULT_SIZE;
  min.y = -DEFAULT_SIZE;
  min.z = -DEFAULT_SIZE;

  max.x = DEFAULT_SIZE;
  max.y = DEFAULT_SIZE;
  max.z = DEFAULT_SIZE;
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(glm::vec3 const& minp, glm::vec3 const& maxp)
    : min(minp)
    , max(maxp)
{
}

glm::vec3
AxisAlignedBoundingBox::dimensions() const
{
  auto const w = max.x - min.x;
  auto const h = max.y - min.y;
  auto const l = max.z - min.z;
  return glm::vec3{w, h, l};
}

glm::vec3
AxisAlignedBoundingBox::center() const
{
  return half_widths();
}

glm::vec3
AxisAlignedBoundingBox::half_widths() const
{
  return dimensions() / glm::vec3{2.0f};
}

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
