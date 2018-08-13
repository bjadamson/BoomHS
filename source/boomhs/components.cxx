#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/math.hpp>

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
  return math::calculate_cube_dimensions(min, max);
}

glm::vec3
AABoundingBox::center() const
{
  auto const hw = half_widths();
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

glm::vec3
AABoundingBox::scaled_min(Transform const& tr) const
{
  auto const s  = tr.scale;
  auto const c  = center();
  auto const hw = half_widths();

  auto r = this->min;
  r.x = c.x - (s.x * hw.x);
  r.y = c.y - (s.y * hw.y);
  r.z = c.z - (s.z * hw.z);
  return r;
}

glm::vec3
AABoundingBox::scaled_max(Transform const& tr) const
{
  auto const s  = tr.scale;
  auto const c  = center();
  auto const hw = half_widths();

  auto r = this->max;
  r.x = c.x + (s.x * hw.x);
  r.y = c.y + (s.y * hw.y);
  r.z = c.z + (s.z * hw.z);
  return r;
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
