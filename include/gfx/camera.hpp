#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stlw/type_macros.hpp>
#include <gfx/skybox.hpp>

namespace gfx
{

class camera
{
  skybox skybox_;
  glm::vec3 pos_;
  glm::vec3 front_;
  glm::vec3 up_;

  glm::quat raw, pitch, roll;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // immutable helper methods
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

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating helper methods
  decltype(auto) move_along_x(float const s)
  {
    this->pos_ += right_vector(s);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  decltype(auto) move_along_y(float const s)
  {
    this->pos_ += (this->up_ * s);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

  decltype(auto) move_along_z(float const s)
  {
    this->pos_ += direction(s);
    this->skybox_.model.translation = this->pos_;
    return *this;
  }

public:
  MOVE_CONSTRUCTIBLE_ONLY(camera);

  camera(skybox &&sb, glm::vec3 const& pos, glm::vec3 const& front, glm::vec3 const& up)
    : skybox_(std::move(sb))
    , pos_(pos)
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
    auto &pos = this->pos_;
    auto const new_front = pos + this->front_;
    return glm::lookAt(pos, new_front, this->up_);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // mutating methods
  //
  // linear movement
  camera& move_forward(float const s)
  {
    return move_along_z(-s);
  }

  camera& move_backward(float const s)
  {
    return move_along_z(s);
  }

  camera& move_left(float const s)
  {
    return move_along_x(-s);
  }

  camera& move_right(float const s)
  {
    return move_along_x(s);
  }

  camera& move_up(float const s)
  {
    return move_along_y(-s);
  }

  camera& move_down(float const s)
  {
    return move_along_y(s);
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
  /*
  camera& move_mouse_to(float x, float y, float const xdest, float const ydest)
  {
    //if (firstMouse) {
      //lastX = xpos;
      //lastY = ypos;
      //firstMouse = false;
    //}
    GLfloat xoffset = xdest - x;
    GLfloat yoffset = ydest - y;

    x = xdest;
    y = ydest;

    GLfloat const sensitivity = 0.05;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    this->yaw   += xoffset;
    this->pitch += yoffset;

    if (this->pitch > 89.0f) {
      this->pitch = 89.0f;
    }
    if (this->pitch < -89.0f) {
      this->pitch = -89.0f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->front_ = glm::normalize(front);
  }
  */
};

struct camera_factory
{
  static auto make_default(model &skybox_model)
  {
    return gfx::camera{skybox{skybox_model},
      glm::vec3(0.0f, 0.0f, 2.0f),  // camera position
      glm::vec3(0.0f, 0.0f, -1.0f), // look at origin
      glm::vec3(0.0f, 1.0f, 0.0f)}; // "up" vector
  }
};

} // ns gfx
