#pragma once
#include <boomhs/math.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{
struct AABoundingBox;
class Transform;

class ViewFrustum
{
  // This holds the A B C and D values for each side of our frustum.
  Plane planes_[6];

public:
  NO_MOVE(ViewFrustum);
  NO_COPY(ViewFrustum);

  ViewFrustum() = default;

  // Call this every time the camera moves to update the frustum
  void recalculate(glm::mat4 const&, glm::mat4 const&);

  // This takes the center and half the length of the cube.
  bool cube_in_frustum(float, float, float, float size) const;
  bool cube_in_frustum(glm::vec3 const&, float size) const;

  static bool
  bbox_inside(glm::mat4 const&, glm::mat4 const&, Transform const&, AABoundingBox const&);
};

} // namespace boomhs
