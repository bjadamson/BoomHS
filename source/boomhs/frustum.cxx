#include <boomhs/frustum.hpp>
#include <opengl/frame.hpp>

using namespace boomhs;
using namespace opengl;

// Algorithm(s) adopted/modified from:
// https://github.com/gametutorials/tutorials/blob/master/OpenGL/Frustum%20Culling/Frustum.cpp

namespace
{

void
normalize_plane(float frustum[6][4], int const side)
{
  // Here we calculate the magnitude of the normal to the plane (point A B C)
  // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
  // To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
  float magnitude =
      (float)sqrt(frustum[side][A] * frustum[side][A] + frustum[side][B] * frustum[side][B] +
                  frustum[side][C] * frustum[side][C]);

  // Then we divide the plane's values by it's magnitude.
  // This makes it easier to work with.
  frustum[side][A] /= magnitude;
  frustum[side][B] /= magnitude;
  frustum[side][C] /= magnitude;
  frustum[side][D] /= magnitude;
}

} // namespace

namespace boomhs
{

void
Frustum::recalculate(FrameState const& fstate)
{
  glm::mat4 const& proj_mat = fstate.projection_matrix();
  glm::mat4 const& view_mat = fstate.view_matrix();

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
  data_[RIGHT][A] = clip[3] - clip[0];
  data_[RIGHT][B] = clip[7] - clip[4];
  data_[RIGHT][C] = clip[11] - clip[8];
  data_[RIGHT][D] = clip[15] - clip[12];

  // Now that we have a normal (A,B,C) and a distance (D) to the plane,
  // we want to normalize that normal and distance.

  // Normalize the RIGHT side
  normalize_plane(data_, RIGHT);

  // This will extract the LEFT side of the frustum
  data_[LEFT][A] = clip[3] + clip[0];
  data_[LEFT][B] = clip[7] + clip[4];
  data_[LEFT][C] = clip[11] + clip[8];
  data_[LEFT][D] = clip[15] + clip[12];

  // Normalize the LEFT side
  normalize_plane(data_, LEFT);

  // This will extract the BOTTOM side of the frustum
  data_[BOTTOM][A] = clip[3] + clip[1];
  data_[BOTTOM][B] = clip[7] + clip[5];
  data_[BOTTOM][C] = clip[11] + clip[9];
  data_[BOTTOM][D] = clip[15] + clip[13];

  // Normalize the BOTTOM side
  normalize_plane(data_, BOTTOM);

  // This will extract the TOP side of the frustum
  data_[TOP][A] = clip[3] - clip[1];
  data_[TOP][B] = clip[7] - clip[5];
  data_[TOP][C] = clip[11] - clip[9];
  data_[TOP][D] = clip[15] - clip[13];

  // Normalize the TOP side
  normalize_plane(data_, TOP);

  // This will extract the BACK side of the frustum
  data_[BACK][A] = clip[3] - clip[2];
  data_[BACK][B] = clip[7] - clip[6];
  data_[BACK][C] = clip[11] - clip[10];
  data_[BACK][D] = clip[15] - clip[14];

  // Normalize the BACK side
  normalize_plane(data_, BACK);

  // This will extract the FRONT side of the frustum
  data_[FRONT][A] = clip[3] + clip[2];
  data_[FRONT][B] = clip[7] + clip[6];
  data_[FRONT][C] = clip[11] + clip[10];
  data_[FRONT][D] = clip[15] + clip[14];

  // Normalize the FRONT side
  normalize_plane(data_, FRONT);
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

  for (int i = 0; i < 6; i++) {
    // clang-format off
    if (data_[i][A] * (x - size) + data_[i][B] * (y - size) + data_[i][C] * (z - size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x + size) + data_[i][B] * (y - size) + data_[i][C] * (z - size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x - size) + data_[i][B] * (y + size) + data_[i][C] * (z - size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x + size) + data_[i][B] * (y + size) + data_[i][C] * (z - size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x - size) + data_[i][B] * (y - size) + data_[i][C] * (z + size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x + size) + data_[i][B] * (y - size) + data_[i][C] * (z + size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x - size) + data_[i][B] * (y + size) + data_[i][C] * (z + size) + data_[i][D] > 0)
      continue;
    if (data_[i][A] * (x + size) + data_[i][B] * (y + size) + data_[i][C] * (z + size) + data_[i][D] > 0)
      continue;
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

} // namespace boomhs
