#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{
class Transform;

struct Ray
{
  glm::vec3 const& orig;
  glm::vec3 const& dir;
  glm::vec3 const  invdir;

  std::array<int, 3> const sign;

  explicit Ray(glm::vec3 const&, glm::vec3 const&);
};

} // namespace boomhs

namespace boomhs::collision
{

bool
box_intersect(Ray const&, Transform const&, AABoundingBox const&);

} // namespace boomhs::collision
