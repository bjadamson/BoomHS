#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx
{

class camera
{
  glm::vec3 pos_;
  glm::vec3 front_;
  glm::vec3 up_;

  auto direction(float const speed) const
  {
    return speed * this->front_;
  }

  auto right_vector(float const speed) const
  {
    return glm::normalize(glm::cross(this->front_, this->up_)) * speed;
  }

  auto xpan(float const d) const
  {
    return glm::vec3{d, 0.0f, 0.0f};
  }

  auto ypan(float const d) const
  {
    return glm::vec3{0.0f, -d, 0.0f};
  }

  friend struct camera_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(camera);

  camera(glm::vec3 const& pos, glm::vec3 const& front, glm::vec3 const& up)
    : pos_(pos)
    , front_(front)
    , up_(up)
  {}

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable methods
  auto
  compute_view() const
  {
    auto &pos = this->pos_;
    auto const new_front = pos + this->front_;
    return glm::lookAt(pos, new_front, this->up_);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating methods
  //
  // linar movement
  camera& move_forward(float const s)
  {
    this->pos_ += direction(s);
    return *this;
  }

  camera& move_backward(float const s)
  {
    this->pos_ += direction(-s);
    return *this;
  }

  camera& move_left(float const s)
  {
    this->pos_ += right_vector(s);
    return *this;
  }

  camera& move_right(float const s)
  {
    this->pos_ += right_vector(-s);
    return *this;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // pan-movement
  camera& pan_up(float const d)
  {
    this->pos_ += ypan(d);
    return *this;
  }

  camera& pan_down(float const d)
  {
    this->pos_ += ypan(-d);
    return *this;
  }

  camera& pan_left(float const d)
  {
    this->pos_ += xpan(d);
    return *this;
  }

  camera& pan_right(float const d)
  {
    this->pos_ += xpan(-d);
    return *this;
  }
};


} // ns engine::gfx
