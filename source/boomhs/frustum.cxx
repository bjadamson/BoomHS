#include <boomhs/frustum.hpp>
#include <boomhs/components.hpp>
#include <boomhs/frame.hpp>
#include <cmath>

using namespace boomhs;
using namespace opengl;

// Algorithm(s) adopted/modified from:
// https://github.com/gametutorials/tutorials/blob/master/OpenGL/Frustum%20Culling/Frustum.cpp

namespace
{

void
normalize_plane(Plane planes[6], int const side)
{
  // Here we calculate the magnitude of the normal to the plane (point A B C)
  // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
  // To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
  float const magnitude = std::sqrt(
        planes[side].a * planes[side].a
      + planes[side].b * planes[side].b
      + planes[side].c * planes[side].c
      );

  // Then we divide the plane's values by it's magnitude.
  // This makes it easier to work with.
  planes[side].a /= magnitude;
  planes[side].b /= magnitude;
  planes[side].c /= magnitude;
  planes[side].d /= magnitude;
}

} // namespace

namespace boomhs
{

void
Frustum::recalculate(glm::mat4 const& view_mat, glm::mat4 const& proj_mat)
{
  auto const& view = glm::value_ptr(view_mat);
  auto const& proj = glm::value_ptr(proj_mat);

  // Now that we have our modelview and projection matrix, if we combine these 2 matrices,
  // it will give us our clipping planes.  To combine 2 matrices, we multiply them.
  float clip[16];
  clip[0] = view[0] * proj[0] + view[1] * proj[4] + view[2] * proj[8] + view[3] * proj[12];
  clip[1] = view[0] * proj[1] + view[1] * proj[5] + view[2] * proj[9] + view[3] * proj[13];
  clip[2] = view[0] * proj[2] + view[1] * proj[6] + view[2] * proj[10] + view[3] * proj[14];
  clip[3] = view[0] * proj[3] + view[1] * proj[7] + view[2] * proj[11] + view[3] * proj[15];

  clip[4] = view[4] * proj[0] + view[5] * proj[4] + view[6] * proj[8] + view[7] * proj[12];
  clip[5] = view[4] * proj[1] + view[5] * proj[5] + view[6] * proj[9] + view[7] * proj[13];
  clip[6] = view[4] * proj[2] + view[5] * proj[6] + view[6] * proj[10] + view[7] * proj[14];
  clip[7] = view[4] * proj[3] + view[5] * proj[7] + view[6] * proj[11] + view[7] * proj[15];

  clip[8]  = view[8] * proj[0] + view[9] * proj[4] + view[10] * proj[8] + view[11] * proj[12];
  clip[9]  = view[8] * proj[1] + view[9] * proj[5] + view[10] * proj[9] + view[11] * proj[13];
  clip[10] = view[8] * proj[2] + view[9] * proj[6] + view[10] * proj[10] + view[11] * proj[14];
  clip[11] = view[8] * proj[3] + view[9] * proj[7] + view[10] * proj[11] + view[11] * proj[15];

  clip[12] = view[12] * proj[0] + view[13] * proj[4] + view[14] * proj[8] + view[15] * proj[12];
  clip[13] = view[12] * proj[1] + view[13] * proj[5] + view[14] * proj[9] + view[15] * proj[13];
  clip[14] = view[12] * proj[2] + view[13] * proj[6] + view[14] * proj[10] + view[15] * proj[14];
  clip[15] = view[12] * proj[3] + view[13] * proj[7] + view[14] * proj[11] + view[15] * proj[15];

  // Now we actually want to get the sides of the frustum.  To do this we take
  // the clipping planes we received above and extract the sides from them.

  // This will extract the RIGHT side of the frustum
  planes_[RIGHT].a = clip[3]  - clip[0];
  planes_[RIGHT].b = clip[7]  - clip[4];
  planes_[RIGHT].c = clip[11] - clip[8];
  planes_[RIGHT].d = clip[15] - clip[12];

  // Now that we have a normal (A,B,C) and a distance (D) to the plane,
  // we want to normalize that normal and distance.

  // Normalize the RIGHT side
  normalize_plane(planes_, RIGHT);

  // This will extract the LEFT side of the frustum
  planes_[LEFT].a = clip[3]  + clip[0];
  planes_[LEFT].b = clip[7]  + clip[4];
  planes_[LEFT].c = clip[11] + clip[8];
  planes_[LEFT].d = clip[15] + clip[12];

  // Normalize the LEFT side
  normalize_plane(planes_, LEFT);

  // This will extract the BOTTOM side of the frustum
  planes_[BOTTOM].a = clip[3] + clip[1];
  planes_[BOTTOM].b = clip[7] + clip[5];
  planes_[BOTTOM].c = clip[11] + clip[9];
  planes_[BOTTOM].d = clip[15] + clip[13];

  // Normalize the BOTTOM side
  normalize_plane(planes_, BOTTOM);

  // This will extract the TOP side of the frustum
  planes_[TOP].a = clip[3] - clip[1];
  planes_[TOP].b = clip[7] - clip[5];
  planes_[TOP].c = clip[11] - clip[9];
  planes_[TOP].d = clip[15] - clip[13];

  // Normalize the TOP side
  normalize_plane(planes_, TOP);

  // This will extract the BACK side of the frustum
  planes_[BACK].a = clip[3] - clip[2];
  planes_[BACK].b = clip[7] - clip[6];
  planes_[BACK].c = clip[11] - clip[10];
  planes_[BACK].d = clip[15] - clip[14];

  // Normalize the BACK side
  normalize_plane(planes_, BACK);

  // This will extract the FRONT side of the frustum
  planes_[FRONT].a = clip[3] + clip[2];
  planes_[FRONT].b = clip[7] + clip[6];
  planes_[FRONT].c = clip[11] + clip[10];
  planes_[FRONT].d = clip[15] + clip[14];

  // Normalize the FRONT side
  normalize_plane(planes_, FRONT);
}

bool
Frustum::cube_in_frustum(float const x, float const y, float const z, float const size) const
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
      glm::vec3{x - size, y - size, z - size},
      glm::vec3{x + size, y - size, z - size},
      glm::vec3{x - size, y + size, z - size},
      glm::vec3{x + size, y + size, z - size},

      glm::vec3{x - size, y - size, z + size},
      glm::vec3{x + size, y - size, z + size},
      glm::vec3{x - size, y + size, z + size},
      glm::vec3{x + size, y + size, z + size}
      );

  FOR(i, 6) {
    auto const& p = planes_;
    // clang-format off
    if (Plane::dotproduct_with_vec3(p[i], points[0]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[1]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[2]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[3]) >= 0) continue;

    if (Plane::dotproduct_with_vec3(p[i], points[4]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[5]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[6]) >= 0) continue;
    if (Plane::dotproduct_with_vec3(p[i], points[7]) >= 0) continue;

    // clang-format on

    // If we get here, it isn't in the frustum
    return false;
  }

  return true;
}

bool
Frustum::cube_in_frustum(glm::vec3 const& pos, float const size) const
{
  return cube_in_frustum(pos.x, pos.y, pos.z, size);
}

bool
Frustum::bbox_inside(glm::mat4 const& view_mat, glm::mat4 const& proj_mat, Transform const& tr,
                     AABoundingBox const& bbox)
{
  // TODO: only call recalulate when the camera moves
  Frustum frust;
  frust.recalculate(view_mat, proj_mat);

  auto const& cube            = bbox.cube;
  float const halfsize        = glm::length(cube.max - cube.min) / 2.0f;
  bool const  bbox_in_frustum = frust.cube_in_frustum(tr.translation, halfsize);
  return bbox_in_frustum;
}

} // namespace boomhs
