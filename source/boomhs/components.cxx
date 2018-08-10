#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/leveldata.hpp>
#include <stlw/math.hpp>

namespace boomhs
{

AABoundingBox::AABoundingBox()
{
  auto constexpr DEFAULT_SIZE = 1.0f;

  min.x = -DEFAULT_SIZE;
  min.y = -DEFAULT_SIZE;
  min.z = -DEFAULT_SIZE;

  max.x = DEFAULT_SIZE;
  max.y = DEFAULT_SIZE;
  max.z = DEFAULT_SIZE;
}

AABoundingBox::AABoundingBox(glm::vec3 const& minp, glm::vec3 const& maxp)
    : min(minp)
    , max(maxp)
{
}

glm::vec3
AABoundingBox::dimensions() const
{
  return stlw::math::calculate_cube_dimensions(min, max);
}

glm::vec3
AABoundingBox::center() const
{
  auto const check = [](float const a, float const b) { assert(stlw::math::float_compare(a, b)); };
  auto const hw = half_widths();
  check(min.x + hw.x, max.x - hw.x);
  check(min.y + hw.y, max.y - hw.y);
  check(min.z + hw.z, max.z - hw.z);

  return glm::vec3{
    min.x + hw.x,
    min.y + hw.y,
    min.z + hw.z
  };
}

glm::vec3
AABoundingBox::half_widths() const
{
  return dimensions() / glm::vec3{2.0f};
}

void
AABoundingBox::add_to_entity(EntityID const eid, EntityRegistry& registry, glm::vec3 const& min,
                             glm::vec3 const& max)
{
  auto& bbox = registry.assign<AABoundingBox>(eid);
  bbox.min   = min;
  bbox.max   = max;
  registry.assign<Selectable>(eid);
}

} // namespace boomhs
