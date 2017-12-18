#pragma once
#include <glm/glm.hpp>
#include <opengl/types.hpp>
#include <opengl/camera.hpp>

namespace boomhs
{

class Player
{
  opengl::Model &model_;
  glm::vec3 front_, up_;
  glm::quat orientation_;

  auto& move(float const s, glm::vec3 const& dir)
  {
    model_.translation += (dir * s);
    return *this;
  }

  //auto& move_z(float const s)
  //{
    //return move(s, opengl::Z_UNIT_VECTOR);
  //}

  //auto& move_x(float const s)
  //{
    //return move(s, opengl::X_UNIT_VECTOR);
  //}

  //auto& move_y(float const s)
  //{
    //return move(s, opengl::Y_UNIT_VECTOR);
  //}

  auto right_vector() const
  {
    return glm::normalize(glm::cross(this->front_, this->up_));
  }

  auto back_vector() const { return -front_; }
  auto left_vector() const { return -right_vector(); }
  auto down_vector() const { return -up_; }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Player);
  explicit Player(opengl::Model &m, glm::vec3 const& front, glm::vec3 const& up)
    : model_(m)
    , front_(front)
    , up_(up)
  {
  }

  auto& move_forward(float const s)
  {
    return move(s, -front_);
  }

  auto& move_backward(float const s)
  {
    return move(s, front_);
  }

  auto& move_left(float const s)
  {
    return move(s, left_vector());
  }

  auto& move_right(float const s)
  {
    return move(s, right_vector());
  }

  auto& move_up(float const s)
  {
    return move(s, up_);
  }

  auto& move_down(float const s)
  {
    return move(s, down_vector());
  }

  void
  set_orientation(glm::quat const& o)
  {
   model_.rotation = o;
  }
};

} // ns boomhs
