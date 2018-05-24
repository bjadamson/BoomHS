#pragma once
#include <boomhs/components.hpp>

namespace boomhs
{
class Transform;

struct Ray
{
  glm::vec3 const& orig;
  glm::vec3 const& dir;
  glm::vec3 const  invdir;

  std::array<int, 3> const sign;

  explicit Ray(glm::vec3 const& o, glm::vec3 const& d)
      : orig(o)
      , dir(d)
      , invdir(1.0f / dir)
      , sign(stlw::make_array<int>(invdir.x < 0, invdir.y < 0, invdir.z < 0))
  {
  }
};

} // namespace boomhs

namespace boomhs::collision
{

bool
box_intersect(Ray const&, Transform const&, AABoundingBox const&);

} // namespace boomhs::collision
