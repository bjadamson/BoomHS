#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <opengl/skybox.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <window/mouse.hpp>

namespace boomhs
{
struct UiState;
} // ns boomhs

namespace opengl
{

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

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

class OrbitCamera
{
  glm::vec3 up_;
  SphericalCoordinates coordinates_{1.0f, -2.608, 0.772};

public:
  MOVE_CONSTRUCTIBLE_ONLY(OrbitCamera);
  OrbitCamera(glm::vec3 const& up)
    : up_(up)
  {
  }

  auto const& up() const { return this->up_; }
  std::string display() const;

  glm::quat orientation() const { return to_cartesian(coordinates_); }
  glm::mat4 view(Model const&) const;

  OrbitCamera&
  zoom(float const);

  OrbitCamera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);
};

class Camera
{
  Projection const projection_;
  skybox skybox_;

  OrbitCamera orbit_;

  Model &target_;

  glm::vec3 const&
  up() const
  {
    return orbit_.up();
  }

  glm::mat4 projection() const
  {
    auto const& p = projection_;
    auto const fov = glm::radians(p.field_of_view);
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  Camera(Projection const&, skybox &&, glm::vec3 const& front, glm::vec3 const& up, Model &);

  glm::quat
  orientation() const
  {
    return orbit_.orientation();
  }

  glm::mat4
  view() const
  {
    return orbit_.view(target_);
  }

  glm::mat4
  matrix() const
  {
    return projection() * view();
  }

  void
  set_follow_target(Model &m)
  {
    this->target_ = m;
  }

  std::string display() const;
  std::string follow_target_display() const;

  Camera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);

  Camera&
  zoom(float const distance)
  {
    orbit_.zoom(distance);
    return *this;
  }

  //Camera&
  //move(float const s, glm::vec3 const& dir)
  //{
    //auto const& front = this->front();
    //set_front(front + (dir * s));

    //this->skybox_.model.translation = front;

    //return *this;
  //}
};

struct CameraFactory
{
  static auto make_default(Projection const& proj, Model &skybox_model,
      Model &target, glm::vec3 const& forward, glm::vec3 const& up)
  {
    return opengl::Camera{proj, skybox{skybox_model}, forward, up, target};
  }
};

} // ns opengl
