#pragma once
#include <boomhs/types.hpp>
#include <window/mouse.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{

class Player
{
  boomhs::Transform &transform_;
  boomhs::Transform &arrow_;
  glm::vec3 forward_, up_;

  auto&
  move(float const s, glm::vec3 const& dir)
  {
    transform_.translation += (s * dir);
    arrow_.translation += (s * dir);
    return *this;
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(Player);
  explicit Player(boomhs::Transform &m, boomhs::Transform &a, glm::vec3 const& forward, glm::vec3 const& up)
    : transform_(m)
    , arrow_(a)
    , forward_(forward)
    , up_(up)
  {
  }

  glm::vec3
  right_vector() const
  {
    auto const cross = glm::cross(forward_vector(), up_vector());
    return glm::normalize(cross);
  }

  auto back_vector() const { return -forward_vector(); }
  auto left_vector() const { return -right_vector(); }
  auto down_vector() const { return -up_vector(); }

  std::string
  display() const;

  glm::vec3
  forward_vector() const
  {
    return forward_ * orientation();
  }

  glm::vec3
  up_vector() const
  {
    return up_ * orientation();
  }

  glm::quat const&
  orientation() const
  {
    return transform_.rotation;
  }

  glm::vec3 const&
  world_position() const
  {
    return transform_.translation;
  }

  auto& move_forward(float const s)
  {
    return move(s, forward_vector());
  }

  auto& move_backward(float const s)
  {
    return move(s, back_vector());
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
    return move(s, up_vector());
  }

  auto& move_down(float const s)
  {
    return move(s, down_vector());
  }

  void
  rotate(float const, window::mouse_data const&);

  void
  multiply_quat(glm::quat const&);
};

} // ns boomhs
