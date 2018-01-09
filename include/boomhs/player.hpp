#pragma once
#include <boomhs/types.hpp>
#include <window/mouse.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{

class Player
{
  boomhs::Transform *transform_;
  glm::vec3 forward_, up_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(Player);
  explicit Player(boomhs::Transform &m, glm::vec3 const& forward, glm::vec3 const& up)
    : transform_(&m)
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

  auto backward_vector() const { return -forward_vector(); }
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
    return transform_->rotation;
  }

  glm::vec3 const&
  world_position() const
  {
    return transform_->translation;
  }

  auto&
  move(float const s, glm::vec3 const& dir)
  {
    transform_->translation += (s * dir);
    return *this;
  }

  void
  rotate(float const, glm::vec3 const&);

  auto
  tilemap_position() const
  {
    auto const& pos = transform_->translation;

    // Truncate the floating point values to get tilemap position
    auto const trunc = [](float const v) { return abs(v); };
    return glm::vec3{trunc(pos.x), trunc(pos.y), trunc(pos.z)};
  }

  void
  move_to(glm::vec3 const& pos)
  {
    transform_->translation = pos;
  }

  void
  set_transform(Transform &transform)
  {
    transform_ = &transform;
  }

  auto const&
  transform() const
  {
    return *transform_;
  }

  glm::mat4
  model_matrix() const { return transform_->model_matrix(); }
};

} // ns boomhs
