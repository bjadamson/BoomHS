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
  float radius_ = 1.5f, theta_, phi_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(OrbitCamera);
  OrbitCamera(glm::vec3 const& front, glm::vec3 const& up)
    : front_(front)
    , up_(up)
  {
  }

  auto const& front() const { return this->front_; }
  auto const& up() const { return this->up_; }

  glm::quat orientation() const { return to_cartesian(radius_, theta_, phi_); }
  glm::mat4 view() const;

  OrbitCamera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);
};

class FpsCamera
{
  glm::vec3 front_, up_;
  glm::quat orientation_;

  float pitch_ = 0.0f, roll_ = 0.0f, yaw_ = 0.0f;

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
  glm::mat4 view() const;

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

  /*
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable
  auto right_vector() const
  {
    return glm::normalize(glm::cross(this->front_, this->up_));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mutation
  auto& move(float const s, glm::vec3 const& dir)
  {
    this->front_ += dir * s;
    this->skybox_.model.translation = this->front_;

    return *this;
  }
  auto& move_z(float const s)
  {
    auto const viewm = view();
    glm::vec3 const forward{viewm[0][2], viewm[1][2], viewm[2][2]};
    return move(s, -forward);
  }

  auto& move_x(float const s)
  {
    auto const viewm = view();
    glm::vec3 const strafe {viewm[0][0], viewm[1][0], viewm[2][0]};
    return move(s, strafe);
  }

  auto& move_y(float const s)
  {
    auto const viewm = view();
    glm::vec3 const updown{viewm[0][1], viewm[1][1], viewm[2][1]};
    return move(s, updown);
  }
  */

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

  glm::mat4
  view() const
  {
    if (active_mode_ == FPS) {
      return fps_.view();
    } else {
      return orbit_.view();
    }
  }

  glm::quat
  orientation() const
  {
    if (active_mode_ == FPS) {
      return fps_.orientation();
    } else {
      return orbit_.orientation();
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
      CameraMode const);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable
  glm::mat4 matrix() const
  {
    return projection() * view();
  }

  glm::vec3
  direction_facing_degrees() const
  {
    return glm::degrees(glm::eulerAngles(this->orientation()));
  }

  void
  set_mode(CameraMode const cmode)
  {
    this->active_mode_ = cmode;
  }

  /*
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mutation
  auto& move_forward(float const s)
  {
    return move_z(s);
  }

  auto& move_backward(float const s)
  {
    return move_z(-s);
  }

  auto& move_left(float const s)
  {
    return move_x(s);
  }

  auto& move_right(float const s)
  {
    return move_x(-s);
  }

  auto& move_up(float const s)
  {
    return move_y(-s);
  }

  auto& move_down(float const s)
  {
    return move_y(s);
  }
  */

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mouse-movement
  Camera&
  rotate(stlw::Logger &, boomhs::UiState &, window::mouse_data const&);
};



struct CameraFactory
{
  static auto make_default(CameraMode const cmode, Projection const& proj, Model &skybox_model)
  {
    auto const& front = -Z_UNIT_VECTOR; // camera-look at origin
    auto const& up = Y_UNIT_VECTOR;     // cameraspace "up" is === "up" in worldspace.
    return opengl::Camera{proj, skybox{skybox_model}, front, up, cmode};
  }
};

} // ns opengl
