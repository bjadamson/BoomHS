#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <stlw/type_macros.hpp>
#include <opengl/skybox.hpp>
#include <window/mouse.hpp>

namespace opengl::detail
{

//inline bool
//between(float const v, float const a, float const b)
//{
  //return ((v <= a) && (v >= b)) || ((v <= b) && (v >= a));
//}

} // ns detail

namespace opengl
{

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

class Camera
{
  Projection const projection_;
  skybox skybox_;
  glm::vec3 front_, up_;

  float pitch_ = 0.0f, roll_ = 0.0f, yaw_ = 0.0f;
  glm::quat orientation_;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable helper methods
  auto right_vector() const
  {
    return glm::normalize(glm::cross(this->front_, this->up_));
  }

  auto xpan(float const d) const
  {
    return glm::vec3{d, 0.0f, 0.0f};
  }

  auto ypan(float const d) const
  {
    return glm::vec3{0.0f, -d, 0.0f};
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating helper methods
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

  glm::vec3 const&
  front() const { return this->front_; }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  Camera(Projection const& proj, skybox &&sb, glm::vec3 const& front, glm::vec3 const& up)
    : projection_(proj)
    , skybox_(MOVE(sb))
    , front_(front)
    , up_(up)
  {
    this->skybox_.model.translation = this->front_;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable methods
  glm::quat const&
  orientation() const { return this->orientation_; }

  glm::mat4 projection() const
  {
    auto const& p = this->projection_;
    auto const fov = glm::radians(p.field_of_view);
    return glm::perspective(fov, p.viewport_aspect_ratio, p.near_plane, p.far_plane);
  }

  glm::mat4 matrix() const
  {
    return projection() * view();
  }

  glm::mat4 view() const
  {
    glm::vec3 const pos = -front();
    auto const translation = glm::translate(glm::mat4(), pos);

    glm::mat4 const orientation = glm::mat4_cast(this->orientation());
    return orientation * translation;
  }

  glm::vec3
  direction_facing_degrees() const
  {
    return glm::degrees(glm::eulerAngles(this->orientation()));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating methods
  //
  // linear movement
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

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mouse-movement
  template<typename L>
  auto& rotate_to(L &logger, window::mouse_data const& mdata)
  {
    auto const& current = mdata.current;
    auto const& mouse_sens = mdata.sensitivity;

    glm::vec2 const delta = glm::vec2{current.xrel, current.yrel};

    auto const yaw = mouse_sens.x * delta.x;
    auto const pitch = mouse_sens.y * delta.y;
    auto const roll = this->roll_;

    bool const moving_down = current.yrel >= 0;
    bool const moving_up = current.yrel <= 0;

    auto const new_pitch = glm::degrees(this->pitch_ + pitch);
    if (mdata.pitch_lock) {
      if(new_pitch > 0.0f && moving_down) {
        LOG_ERROR("DOWN LOCK");
        return *this;
      }
      if(new_pitch < -45.0f && moving_up) {
        LOG_ERROR("UP LOCK");
        return *this;
      }
    }

    this->yaw_ += yaw;
    this->pitch_ += pitch;

    auto const quat = glm::quat(glm::vec3{pitch, yaw, roll});
    this->orientation_ = glm::normalize(quat * this->orientation_);
    return *this;
  }
};

struct CameraFactory
{
  static auto make_default(Projection const& proj, Model &skybox_model)
  {
    auto const& front = -Z_UNIT_VECTOR; // camera-look at origin
    auto const& up = Y_UNIT_VECTOR;     // cameraspace "up" is === "up" in worldspace.
    return opengl::Camera{proj, skybox{skybox_model}, front, up};
  }
};

} // ns opengl
