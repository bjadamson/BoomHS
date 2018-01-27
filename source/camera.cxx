#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/constants.hpp>
#include <window/mouse.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <cmath>
#include <limits>

namespace boomhs
{

Camera::Camera(EnttLookup const& player_lookup, glm::vec3 const& forward, glm::vec3 const& up)
  : player_lookup_(player_lookup)
  , forward_(forward)
  , up_(up)
  , coordinates_(0.0f, 0.0f, 0.0f)
  , perspective_({90.0f, 4.0f / 3.0f, 0.1f, 200.0f})
  , ortho_({-10, 10, -10, 10, -200, 200})
{
}

glm::mat4
Camera::projection_matrix() const
{
  auto const& p = perspective_;
  auto const fov = glm::radians(p.field_of_view);
  switch(mode_) {
    case Perspective: {
      return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
    }
    case Ortho: {
      auto const& o = ortho_;
      return glm::ortho(o.left, o.right, o.bottom, o.top, o.far, o.near);
    }
    case MAX: {
      break;
    }
  }

  std::abort();
  return glm::mat4{}; // appease compiler
}

glm::mat4
Camera::camera_matrix() const
{
  return projection_matrix() * view_matrix();
}

glm::vec3
Camera::forward_vector() const
{
  return glm::normalize(world_position() - target_position());
}

Camera&
Camera::rotate(stlw::Logger &logger, UiState &uistate, glm::vec2 const& delta)
{
  float const d_theta = delta.x;
  float const d_phi = delta.y;

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
  auto const& target = get_target().translation;
  auto const position_xyz = world_position();

  switch (mode_) {
    case Perspective: {
      return glm::lookAt(position_xyz, target, up_);
    }
    case Ortho: {
      return glm::lookAt(
        glm::vec3{0, 0, 0},
        target,
        opengl::Y_UNIT_VECTOR);
    }
    case MAX: {
      break;
    }
  }
  std::abort();
  return glm::mat4{}; // appease compiler
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
