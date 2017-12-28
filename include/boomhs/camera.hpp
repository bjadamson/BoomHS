#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <boomhs/skybox.hpp>
#include <opengl/constants.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <window/mouse.hpp>
#include <string>

namespace boomhs
{
struct UiState;

struct SphericalCoordinates
{
  float radius;
  float theta;
  float phi;

  SphericalCoordinates()
    : radius(1.0f)
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
  glm::vec3 forward_, up_;

  Projection const projection_;
  skybox skybox_;

  Transform &target_;
  glm::mat4 projection() const
  {
    auto const& p = projection_;
    auto const fov = glm::radians(p.field_of_view);
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  Camera(Projection const&, skybox &&, glm::vec3 const& f, glm::vec3 const& u, Transform &);

  glm::quat
  orientation() const
  {
    return to_cartesian(coordinates_);
  }

  glm::mat4
  view() const;

  glm::vec3
  forward_vector() const
  {
    return forward_ * orientation();
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
    return up_ * orientation();
  }

  glm::vec3
  down_vector() const
  {
    return -up_vector();
  }

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
    auto const& target = target_.translation;
    return target + local_position();
  }

  glm::vec3
  target_position() const
  {
    return this->target_.translation;
  }

  glm::mat4
  matrix() const
  {
    return projection() * view();
  }

  void
  set_follow_target(Transform &m)
  {
    this->target_ = m;
  }

  Camera&
  rotate(stlw::Logger &, UiState &, window::mouse_data const&);

  Camera&
  zoom(float const);
  //Camera&
  //move(float const s, glm::vec3 const& dir)
  //{
    //auto const& front = this->front();
    //set_front(front + (dir * s));

    //this->skybox_.transform.translation = front;

    //return *this;
  //}
};

struct CameraFactory
{
  static auto make_default(Projection const& proj, Transform &skybox_transform,
      Transform &target, glm::vec3 const& forward, glm::vec3 const& up)
  {
    return Camera{proj, skybox{skybox_transform}, forward, up, target};
  }
};

} // ns boomhs
