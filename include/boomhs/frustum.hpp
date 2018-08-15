#pragma once
#include <common/type_macros.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{
struct AABoundingBox;
struct Transform;

// We create an enum of the sides so we don't have to call each side 0 or 1.
// This way it makes it more understandable and readable when dealing with frustum sides.
enum FrustumSide
{
  RIGHT  = 0, // The RIGHT side of the frustum
  LEFT   = 1, // The LEFT	 side of the frustum
  BOTTOM = 2, // The BOTTOM side of the frustum
  TOP    = 3, // The TOP side of the frustum
  BACK   = 4, // The BACK	side of the frustum
  FRONT  = 5  // The FRONT side of the frustum
};

// Like above, instead of saying a number for the ABC and D of the plane, we
// want to be more descriptive.
enum PlaneData
{
  A = 0, // The X value of the plane's normal
  B = 1, // The Y value of the plane's normal
  C = 2, // The Z value of the plane's normal
  D = 3  // The distance the plane is from the origin
};

class Frustum
{
  // This holds the A B C and D values for each side of our frustum.
  float data_[6][4];

public:
  NO_MOVE(Frustum);
  NO_COPY(Frustum);

  Frustum() = default;

  // Call this every time the camera moves to update the frustum
  void recalculate(glm::mat4 const&, glm::mat4 const&);

  // This takes the center and half the length of the cube.
  bool cube_in_frustum(float, float, float, float size) const;
  bool cube_in_frustum(glm::vec3 const&, float size) const;

  static bool bbox_inside(glm::mat4 const&, glm::mat4 const&, Transform const&,
                          AABoundingBox const&);
};

} // namespace boomhs
