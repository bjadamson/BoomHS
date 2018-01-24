#include <boomhs/spherical.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <cmath>

namespace boomhs
{

glm::vec3
to_cartesian(SphericalCoordinates const& coords)
{
  float const radius = coords.radius;
  float const theta = coords.theta;
  float const phi = coords.phi;

  float const sin_phi = sinf(phi);
  float const x = radius * sin_phi * sinf(theta);
  float const y = radius * cosf(phi);
  float const z = radius * sin_phi * cosf(theta);

  //Convert spherical coordinates into Cartesian coordinates
  //float const x = sin(phi) * cos(theta) * radius;
  //float const y = sin(phi) * sin(theta) * radius;
  //float const z = cos(phi) * radius;

  return glm::vec3{x, y, z};
}

SphericalCoordinates
to_spherical(glm::vec3 cartesian)
{
  static constexpr float EPSILONF = std::numeric_limits<float>::epsilon();

  if (cartesian.x == 0) {
    cartesian.x = EPSILONF;
  }
  float const& x = cartesian.x, y = cartesian.y, z = cartesian.z;
  float const x2 = x*x;
  float const y2 = y*y;
  float const z2 = z*z;

  float const radius = sqrt(x2 + y2 + z2);
  float theta = atan(y / x);
  if (cartesian.x < 0) {
    float constexpr PI = glm::pi<float>();
    theta += PI;
  }
  float const phi = atan((x2 + y2) / z);
  //std::cerr << "r: '" << radius << "', theta: '" << theta << "', phi: '" << phi << "'\n";

  return SphericalCoordinates{radius, theta, phi};
}

std::ostream&
operator<<(std::ostream &stream, SphericalCoordinates const& sc)
{
  stream << "{";
  stream << sc.radius;
  stream << ", ";
  stream << sc.theta;
  stream << ", ";
  stream << sc.phi;
  stream << "}";
  return stream;
}

} // ns boomhs
