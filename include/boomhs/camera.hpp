#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <glm/gtx/vector_angle.hpp>
#include <opengl/constants.hpp>
#include <boomhs/types.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <window/mouse.hpp>
#include <cmath>
#include <string>

namespace boomhs
{
class Player;
struct UiState;

struct SphericalCoordinates
{
  float radius;
  float theta;
  float phi;

  SphericalCoordinates()
    : radius(0.0f)
    , theta(0.0f)
    , phi(0.0f)
  {
  }
  SphericalCoordinates(float const r, float const t, float const p)
    : radius(r)
    , theta(t)
    , phi(p)
  {
  }
  explicit SphericalCoordinates(glm::vec3 const& v)
    : SphericalCoordinates(v.x, v.y, v.z)
  {
  }

  std::string
  radius_string() const { return std::to_string(this->radius); }

  std::string
  theta_string() const { return std::to_string(this->theta); }

  std::string
  phi_string() const { return std::to_string(this->phi); }
};

glm::vec3
to_cartesian(SphericalCoordinates const&);

SphericalCoordinates
to_spherical(glm::vec3);

struct Projection
{
  float const field_of_view;
  float const viewport_aspect_ratio;
  float const near_plane;
  float const far_plane;
};

class Camera
{
  SphericalCoordinates coordinates_{1.0f, -2.608, 0.772};
  Projection const projection_;
  Transform &target_;
  glm::vec3 forward_, up_;

  glm::mat4
  projection_matrix() const
  {
    auto const& p = projection_;
    auto const fov = glm::radians(p.field_of_view);
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  }

  glm::mat4
  view_matrix() const;

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  glm::vec3
  forward_vector() const
  {
    // TODO: should be camera_matrix()????
    auto const vm = view_matrix();
    return glm::row(vm, 3);
  }

  Camera(Projection const&, Transform &, glm::vec3 const& f, glm::vec3 const& u);

  glm::mat4
  camera_matrix() const
  {
    return projection_matrix() * view_matrix();
  }

  void
  set_coordinates(SphericalCoordinates const& sc)
  {
    coordinates_ = sc;
  }

  void
  snap_behind_player()
  {
    // desired location is behind the target on the z-axis
    //glm::vec3 const world_pos = player.world_position() + player.back_vector();
    //glm::vec3 const& target_pos = player.world_position();
    //view_ = glm::lookAt(world_pos, target_pos, up_);
  }

  /*
  glm::vec3
  forward_vector() const
  {
    glm::vec3 const& row = view_[2];
    return glm::vec3{row.x, row.y, row.z};
  }

  glm::vec3
  backward_vector() const
  {
    return -forward_vector();
  }

  glm::vec3
  right_vector() const
  {
    auto const cross = glm::cross(forward_vector(), up_vector());
    return glm::normalize(cross);
  }

  glm::vec3
  left_vector() const
  {
    return -right_vector();
  }

  glm::vec3
  up_vector() const
  {
    glm::vec3 const& row = view_[1];
    return glm::vec3{row.x, row.y, row.z};
  }

  glm::vec3
  down_vector() const
  {
    return -up_vector();
  }
  */

  SphericalCoordinates
  spherical_coordinates() const
  {
    return coordinates_;
  }

  glm::vec3
  local_position() const
  {
    return to_cartesian(coordinates_);
  }

  glm::vec3
  world_position() const
  {
    return target_position() + local_position();
  }

  glm::vec3
  target_position() const
  {
    return target_.translation;
  }

  Camera&
  rotate(stlw::Logger &, UiState &, window::mouse_data const&);

  Camera&
  zoom(float const);

  void
  move_behind_player(Player const&);
};

} // ns boomhs
