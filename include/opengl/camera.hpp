#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <stlw/type_macros.hpp>
#include <opengl/skybox.hpp>
#include <window/mouse.hpp>

namespace opengl
{

bool between(float const v, float const a, float const b)
{
  return ((v <= a) && (v >= b)) || ((v <= b) && (v >= a));
}

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

///////////////////////////////////////////////////////////////////////////////////////////////////
// immutable methods
template<typename C>
auto
compute_view(C const& camera)
{
  glm::mat4 const rotate = glm::mat4_cast(camera.orientation());
  auto const translate = glm::translate(glm::mat4{1.0f}, -camera.front());
  return rotate * translate;
}

class Camera
{
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
    auto const mat = compute_view(*this);
    glm::vec3 const forward{mat[0][2], mat[1][2], mat[2][2]};
    return move(s, -forward);
  }

  auto& move_x(float const s)
  {
    auto const mat = compute_view(*this);
    glm::vec3 const strafe {mat[0][0], mat[1][0], mat[2][0]};
    return move(s, strafe);
  }

  auto& move_y(float const s)
  {
    auto const mat = compute_view(*this);
    glm::vec3 const updown{mat[0][1], mat[1][1], mat[2][1]};
    return move(s, updown);
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Camera);

  Camera(skybox &&sb, glm::vec3 const& front, glm::vec3 const& up)
    : skybox_(MOVE(sb))
    , front_(front)
    , up_(up)
  {
    this->skybox_.model.translation = this->front_;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable methods
  glm::quat const&
  orientation() const { return this->orientation_; }

  glm::vec3 const&
  front() const { return this->front_; }

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
  static auto make_default(Model &skybox_model)
  {
    auto const& front = -Z_UNIT_VECTOR; // camera-look at origin
    auto const& up = Y_UNIT_VECTOR;     // cameraspace "up" is === "up" in worldspace.
    return opengl::Camera{skybox{skybox_model}, front, up};
  }
};

} // ns opengl
