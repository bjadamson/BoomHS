#include <boomhs/bounding_object.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/transform.hpp>
#include <boomhs/view_frustum.hpp>

#include <cmath>
#include <common/algorithm.hpp>
#include <common/type_macros.hpp>

using namespace boomhs;
using namespace opengl;

// Algorithm(s) adopted/modified from:
// https://github.com/gametutorials/tutorials/blob/master/OpenGL/Frustum%20Culling/Frustum.cpp

namespace boomhs
{

void
ViewFrustum::recalculate(glm::mat4 const& view_mat, glm::mat4 const& proj_mat)
{
  glm::mat4 const clip = proj_mat * view_mat;

  // Define the center/xyz planes for the frustum
  auto const center = Plane{clip[0][3], clip[1][3], clip[2][3], clip[3][3]};

  auto const xside = Plane{clip[0][0], clip[1][0], clip[2][0], clip[3][0]};
  auto const yside = Plane{clip[0][1], clip[1][1], clip[2][1], clip[3][1]};
  auto const zside = Plane{clip[0][2], clip[1][2], clip[2][2], clip[3][2]};
  // Now we actually want to get the sides of the frustum.  To do this we take
  // the clipping planes we received above and extract the sides from them.
  // Once we have a normal (A,B,C) and a distance (D) to the plane, we want to normalize that
  // normal and distance (for each plane calculated)

  auto& right = planes_[RIGHT];
  right.a     = center.a - xside[0];
  right.b     = center.b - xside[1];
  right.c     = center.c - xside[2];
  right.d     = center.d - xside[3];
  right.normalize();

  auto& left = planes_[LEFT];
  left.a     = center.a + xside[0];
  left.b     = center.b + xside[1];
  left.c     = center.c + xside[2];
  left.d     = center.d + xside[3];
  left.normalize();

  auto& bottom = planes_[BOTTOM];
  bottom.a     = center.a + yside[0];
  bottom.b     = center.b + yside[1];
  bottom.c     = center.c + yside[2];
  bottom.d     = center.d + yside[3];
  bottom.normalize();

  auto& top = planes_[TOP];
  top.a     = center.a - yside[0];
  top.b     = center.b - yside[1];
  top.c     = center.c - yside[2];
  top.d     = center.d - yside[3];
  top.normalize();

  auto& back = planes_[BACK];
  back.a     = center.a - zside[0];
  back.b     = center.b - zside[1];
  back.c     = center.c - zside[2];
  back.d     = center.d - zside[3];
  back.normalize();

  auto& front = planes_[FRONT];
  front.a     = center.a + zside[0];
  front.b     = center.b + zside[1];
  front.c     = center.c + zside[2];
  front.d     = center.d + zside[3];
  front.normalize();
}

bool
ViewFrustum::cube_in_frustum(float const x, float const y, float const z, float const size) const
{
  // This test is a bit more work, but not too much more complicated.
  // Basically, what is going on is, that we are given the center of the cube,
  // and half the length.  Think of it like a radius.  Then we checking each point
  // in the cube and seeing if it is inside the frustum.  If a point is found in front
  // of a side, then we skip to the next side.  If we get to a plane that does NOT have
  // a point in front of it, then it will return false.

  // *Note* - This will sometimes say that a cube is inside the frustum when it isn't.
  // This happens when all the corners of the bounding box are not behind any one plane.
  // This is rare and shouldn't effect the overall rendering speed.
  auto const points = common::make_array<glm::vec3>(
      glm::vec3{x - size, y - size, z - size}, glm::vec3{x + size, y - size, z - size},
      glm::vec3{x - size, y + size, z - size}, glm::vec3{x + size, y + size, z - size},

      glm::vec3{x - size, y - size, z + size}, glm::vec3{x + size, y - size, z + size},
      glm::vec3{x - size, y + size, z + size}, glm::vec3{x + size, y + size, z + size});

  auto const point_within_plane = [](glm::vec3 const& point, Plane const& plane) {
    return Plane::dotproduct_with_vec3(plane, point) >= 0;
  };

  // clang-format off
  FOR(i, 6) {
    auto const& p = planes_[i];
    if (point_within_plane(points[0], p)) { continue; }
    if (point_within_plane(points[1], p)) { continue; }
    if (point_within_plane(points[2], p)) { continue; }
    if (point_within_plane(points[3], p)) { continue; }

    if (point_within_plane(points[4], p)) { continue; }
    if (point_within_plane(points[5], p)) { continue; }
    if (point_within_plane(points[6], p)) { continue; }
    if (point_within_plane(points[7], p)) { continue; }
    // If we get here, it isn't in the frustum
    return false;
  }
  // clang-format on
  return true;
}

bool
ViewFrustum::cube_in_frustum(glm::vec3 const& pos, float const size) const
{
  return cube_in_frustum(pos.x, pos.y, pos.z, size);
}

bool
ViewFrustum::bbox_inside(glm::mat4 const& view_mat, glm::mat4 const& proj_mat, Transform const& tr,
                         AABoundingBox const& bbox)
{
  // TODO: only call recalulate when the camera moves
  ViewFrustum frust;
  frust.recalculate(view_mat, proj_mat);

  auto const& cube            = bbox.cube;
  float const halfsize        = glm::length(cube.max - cube.min) / 2.0f;
  bool const  bbox_in_frustum = frust.cube_in_frustum(tr.translation, halfsize);
  return bbox_in_frustum;
}

} // namespace boomhs
