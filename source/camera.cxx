#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>
#include <limits>
#include <iostream>
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Camera
Camera::Camera(Projection const& proj, Transform &t,glm::vec3 const& forward, glm::vec3 const& up)
  : projection_(proj)
  , target_(&t)
  , forward_(forward)
  , up_(up)
{
}

void
Camera::rotate_behind_player(stlw::Logger &logger, WorldObject const& player)
{
  //auto const& player_transform = player.transform();
  //glm::mat4 const origin_m = glm::translate(glm::mat4{}, glm::vec3{0.0f});
  //glm::mat4 const m = origin_m * glm::toMat4(player.orientation());
  //glm::vec3 player_fwd = origin_m * glm::vec4{player.forward_vector(), 1.0f};
  //player_fwd = glm::normalize(player_fwd);

  //glm::vec3 const camera_fwd = this->forward_vector();
  //float const theta = acos(glm::dot(player_fwd, camera_fwd));
  auto const scoords = to_spherical(player.transform().translation);
  coordinates_.phi = scoords.phi;
  coordinates_.theta = scoords.theta;
  //auto const ccoords = this->spherical_coordinates();
}

Camera&
Camera::rotate(stlw::Logger &logger, UiState &uistate, window::mouse_data const& mdata)
{
  auto const& current = mdata.current;
  glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

  auto const& mouse_sens = mdata.sensitivity;
  float const d_theta = mouse_sens.x * delta.x;
  float const d_phi = mouse_sens.y * delta.y;

  float constexpr PI = glm::pi<float>();
  float constexpr TWO_PI = PI * 2.0f;

  {
    auto const& theta = coordinates_.theta;
    coordinates_.theta = (up_.y > 0.0f) ? (theta - d_theta) : (theta + d_theta);
  }
  {
    auto &theta = coordinates_.theta;
    if (theta > TWO_PI) {
      theta -= TWO_PI;
    } else if (theta < -TWO_PI) {
      theta += TWO_PI;
    }
  }

  auto &phi = coordinates_.phi;
  float const new_phi = uistate.flip_y ? (phi + d_phi) : (phi - d_phi);
  bool const top_hemisphere = (new_phi > 0 && new_phi < (PI/2.0f)) || (new_phi < -(PI/2.0f) && new_phi > -TWO_PI);
  if (!uistate.rotate_lock || top_hemisphere) {
    phi = new_phi;
  }

  // Keep phi within -2PI to +2PI for easy 'up' comparison
  if (phi > TWO_PI) {
    phi -= TWO_PI;
  } else if (phi < -TWO_PI) {
    phi += TWO_PI;
  }

  // If phi is between 0 to PI or -PI to -2PI, make 'up' be positive Y, other wise make it negative Y
  if ((phi > 0 && phi < PI) || (phi < -PI && phi > -TWO_PI)) {
    up_ = opengl::Y_UNIT_VECTOR;
  } else {
    up_ = -opengl::Y_UNIT_VECTOR;
  }
  return *this;
}

glm::mat4
Camera::view_matrix() const
{
  auto const& target = target_->translation;
  auto const position_xyz = world_position();
  return glm::lookAt(position_xyz, target, up_);
}

Camera&
Camera::zoom(float const factor)
{
  float constexpr MIN_RADIUS = 0.01f;
  float const new_radius = coordinates_.radius * factor;
  if (new_radius >= MIN_RADIUS) {
    coordinates_.radius = new_radius;
  } else {
    coordinates_.radius = MIN_RADIUS;
  }
  return *this;
}

} // ns boomhs
