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

glm::vec3
to_cartesian(float const radius, float const theta, float const phi);

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

enum CameraMode
{
  FPS = 0,
  ORBIT,
};

class OrbitCamera
{
  glm::vec3 front_, up_;
  float radius_ = 1.5f, theta_ = -2.33f, phi_ = 0.95f;

public:
  MOVE_CONSTRUCTIBLE_ONLY(OrbitCamera);
  OrbitCamera(glm::vec3 const& front, glm::vec3 const& up)
    : front_(front)
    , up_(up)
  {
  }

  auto const& front() const { return this->front_; }
  auto const& up() const { return this->up_; }
  std::string display() const;

  glm::quat orientation() const { return to_cartesian(radius_, theta_, phi_); }
  glm::mat4 view(Model const&) const;

  void set_front(glm::vec3 const& f) { this->front_ = f; }

  OrbitCamera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);
};

class FpsCamera
{
  glm::vec3 front_, up_;
  glm::quat orientation_;

  float pitch_ = 0.0f, roll_ = 0.0f, yaw_ = 0.0f;

  glm::vec3
  direction_facing_degrees() const;

public:
  MOVE_CONSTRUCTIBLE_ONLY(FpsCamera);
  FpsCamera(glm::vec3 const& front, glm::vec3 const& up)
    : front_(front)
    , up_(up)
  {
  }

  auto const& front() const { return this->front_; }
  auto const& up() const { return this->up_; }

  glm::quat orientation() const { return this->orientation_; }
  glm::mat4 view(Model const&) const;
  std::string display() const;

  void set_front(glm::vec3 const& f) { this->front_ = f; }

  FpsCamera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);
};

class Camera
{
  Projection const projection_;
  CameraMode mode_;
  skybox skybox_;

  FpsCamera fps_;
  OrbitCamera orbit_;

  CameraMode active_mode_;
  Model &target_;

  glm::vec3 const&
  front() const
  {
    if (active_mode_ == FPS) {
      return fps_.front();
    } else {
      return orbit_.front();
    }
  }

  glm::vec3 const&
  up() const
  {
    if (active_mode_ == FPS) {
      return fps_.up();
    } else {
      return orbit_.up();
    }
  }

  glm::mat4 projection() const
  {
    auto const& p = this->projection_;
    auto const fov = glm::radians(p.field_of_view);
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  Camera(Projection const& proj, skybox &&sb, glm::vec3 const& front, glm::vec3 const& up,
      CameraMode const, Model &);

  glm::quat
  orientation() const
  {
    if (active_mode_ == FPS) {
      return fps_.orientation();
    } else {
      return orbit_.orientation();
    }
  }

  glm::mat4
  view() const
  {
    if (active_mode_ == FPS) {
      return fps_.view(target_);
    } else {
      return orbit_.view(target_);
    }
  }
  glm::mat4 matrix() const
  {
    return projection() * view();
  }

  void
  set_follow_target(Model &m)
  {
    this->target_ = m;
  }

  void
  set_mode(CameraMode const cmode)
  {
    this->active_mode_ = cmode;
  }

  void
  set_front(glm::vec3 const& f)
  {
    if (active_mode_ == FPS) {
      return fps_.set_front(f);
    } else {
      return orbit_.set_front(f);
    }
  }

  void
  toggle_mode()
  {
    if (active_mode_ == FPS) {
      active_mode_ = ORBIT;
    } else {
      active_mode_ = FPS;
    }
  }

  std::string display() const;
  std::string follow_target_display() const;

  Camera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);

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
  static auto make_default(CameraMode const cmode, Projection const& proj, Model &skybox_model,
      Model &target, glm::vec3 const& front, glm::vec3 const& up)
  {
    return opengl::Camera{proj, skybox{skybox_model}, front, up, cmode, target};
  }
};

} // ns opengl
