#include <boomhs/math.hpp>
#include <ostream>
#include <utility>

namespace boomhs::math
{

// Original source for algorithm.
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat
rotation_between_vectors(glm::vec3 start, glm::vec3 dest)
{
  using namespace glm;
  start = normalize(start);
  dest  = normalize(dest);

  float const cos_theta = dot(start, dest);
  vec3        axis;
  if (cos_theta < -1 + 0.001f) {
    // special case when vectors in opposite directions:
    // there is no "ideal" rotation axis
    // So guess one; any will do as long as it's perpendicular to start
    axis = cross(vec3(0.0f, 0.0f, 1.0f), start);
    if (glm::length2(axis) < 0.01) {
      // bad luck, they were parallel, try again!
      axis = cross(vec3(1.0f, 0.0f, 0.0f), start);
    }

    axis = normalize(axis);
    return angleAxis(radians(180.0f), axis);
  }
  axis = cross(start, dest);

  float const s    = sqrt((1 + cos_theta) * 2);
  float const invs = 1 / s;

  return glm::quat{s * 0.5f, axis.x * invs, axis.y * invs, axis.z * invs};
}

} // namespace boomhs::math
