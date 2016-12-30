#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <stlw/type_macros.hpp>
#include <gfx/skybox.hpp>
#include <window/mouse.hpp>

namespace gfx
{

bool between(float const v, float const a, float const b)
{
  return ((v <= a) && (v >= b)) || ((v <= b) && (v >= a));
}

auto
angle_between(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& origin)
{
 glm::vec3 da = glm::normalize(a - origin);
 glm::vec3 db = glm::normalize(b - origin);
 return glm::acos(glm::dot(da, db));
}

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

class camera
{
  skybox skybox_;
  glm::vec3 pos_, left_, front_, up_;

  float pitch_ = 0.0f, roll_ = 0.0f, yaw_ = 0.0f;
  glm::quat orientation_;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable helper methods
  auto right_vector() const
  {
    return glm::normalize(glm::cross(this->front_, this->up_));
  }

  // todo: untested
  //auto left_vector() const
  //{
    //return glm::normalize(glm::cross(this->front_, this->right_vector()));
  //}

  auto forward_movement_vector(float const speed) const
  {
    return speed * (this->pos_ * this->orientation_);
  }

  auto sideways_movement_vector(float const speed) const
  {
    return speed * this->right_vector();
  }

  auto updown_movement_vector(float const speed) const
  {
    return speed * this->up_;
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
  decltype(auto) move_sideways(float const s)
  {
    //this->pos_ += sideways_movement_vector(s);
    this->skybox_.model.translation = this->pos_;

    this->front_ = glm::normalize(this->orientation_ * this->pos_);
    return *this;
  }

  decltype(auto) move_updown(float const s)
  {
    //this->pos_ += updown_movement_vector(s);
    this->skybox_.model.translation = this->pos_;

    this->front_ = glm::normalize(this->orientation_ * this->pos_);
    return *this;
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(camera);

  camera(skybox &&sb, glm::vec3 const& pos, glm::vec3 const& left, glm::vec3 const& front,
      glm::vec3 const& up)
    : skybox_(std::move(sb))
    , pos_(pos)
    , left_(left)
    , front_(front)
    , up_(up)
  {
    this->skybox_.model.translation = this->pos_;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable methods
  auto
  compute_view() const
  {
    //FPS camera:  RotationX(pitch) * RotationY(yaw)
    //auto const pitch = glm::angleAxis(this->pitch_, X_UNIT_VECTOR);
    //auto const yaw = glm::angleAxis(this->yaw_, Y_UNIT_VECTOR);
    //auto const roll = glm::angleAxis(this->roll_, Z_UNIT_VECTOR);

    //For a FPS camera we can omit roll
    glm::mat4 const rotate = glm::mat4_cast(this->orientation_);

    auto const translate = glm::translate(glm::mat4{1.0f}, -this->front_);
    return rotate * translate;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating methods
  //
  // linear movement
  decltype(auto) move_forward_impl(float const s)
  {
    auto const dx = 0.2f;
    auto const dz = 0.4f;

    auto const mat = this->compute_view();
    glm::vec3 const forward(mat[0][2], mat[1][2], mat[2][2]);
    glm::vec3 const strafe( mat[0][0], mat[1][0], mat[2][0]);
    this->front_ += (-dz * forward + dx * strafe) * s;
    this->skybox_.model.translation = this->front_;

    return *this;
  }

  camera& move_forward(float const s)
  {
    return move_forward_impl(s);
  }

  camera& move_backward(float const s)
  {
    return move_forward_impl(-s);
  }

  camera& move_left(float const s)
  {
    return move_sideways(s);
  }

  camera& move_right(float const s)
  {
    return move_sideways(-s);
  }

  camera& move_up(float const s)
  {
    return move_updown(-s);
  }

  camera& move_down(float const s)
  {
    return move_updown(s);
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // pan-movement
  camera& pan_up(float const d)
  {
    this->pos_ += ypan(d);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  camera& pan_down(float const d)
  {
    this->pos_ -= ypan(d);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  camera& pan_left(float const d)
  {
    this->pos_ += xpan(d);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  camera& pan_right(float const d)
  {
    this->pos_ -= xpan(d);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mouse-movement
  template<typename L>
  camera& rotate_to(L &logger, window::mouse_state const& mstate)
  {
    glm::vec2 const delta = glm::vec2{mstate.xrel, mstate.yrel};

    //notice that we reduce the sensitvity
    float constexpr mouse_x_sensitivity = 0.0002f;
    float constexpr mouse_y_sensitivity = 0.0002f;

    auto const yaw = mouse_x_sensitivity * delta.x;
    auto const pitch = mouse_y_sensitivity * delta.y;
    auto const roll = this->roll_;

    auto const quat = glm::quat(glm::vec3{pitch, yaw, roll});

    bool const moving_down = mstate.yrel >= 0;
    bool const moving_up = mstate.yrel <= 0;

    auto const o = glm::degrees(glm::eulerAngles(this->orientation_));
    if(o.x > 45.0f && moving_down) {
      logger.error("DOWN LOCK");
      return *this;
    }
        //o.x = 89.0f;
    if(o.x < -45.0f && moving_up) {
      logger.error("UP LOCK");
      return *this;
    }
        //o.x = -89.0f;

    logger.error(fmt::format("quat {} {} {}, pi/2 {}", o.x, o.y, o.z, 3.1459/2));
    this->yaw_ = yaw;
    this->pitch_ = pitch;
    this->orientation_ = glm::normalize(quat * this->orientation_);

    return *this;
  }
};

struct camera_factory
{
  static auto make_default(model &skybox_model)
  {
    glm::vec3 const position = glm::vec3{0.0f, 0.0f, 2.0f};
    return gfx::camera{skybox{skybox_model},
      position,       // camera position
      -X_UNIT_VECTOR, // camera-"left"
      -Z_UNIT_VECTOR, // camera-look at origin
      Y_UNIT_VECTOR}; // camera-"up" vector
  }
};

} // ns gfx
