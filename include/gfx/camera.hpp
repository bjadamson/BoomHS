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

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

class camera
{
  skybox skybox_;
  glm::vec3 pos_, left_;
  glm::vec3 const front_;
  glm::vec3 up_;

  glm::quat orientation_;

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
    auto &pos = this->pos_;
    auto const new_front = pos + (this->orientation_ * this->front_);
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
  template<typename L>
  camera& rotate_to(L &logger, float const, float const, float const xrel, float const yrel)
  {
    auto const calculate_angle = [](float const relative, bool const positive) {
      auto const calc_rate = [](auto const rel) {
        float constexpr TICK_RATE = 60.0f;
        return rel / TICK_RATE;
      };
      auto const right_or_left_multiplier = [](bool const positive) {
        if (positive) {
          return 1.0f;
        } else {
          return -1.0f;
        }
      };
      return glm::radians(right_or_left_multiplier(positive) * calc_rate(relative));
    };
    float const xangle = calculate_angle(xrel, xrel > 800.0f / 2.0f);

    auto const calculate_y = [](auto const rel) {
      float constexpr TICK_RATE = 60.0f;
      return glm::radians(rel / TICK_RATE);
    };
    float yangle = calculate_y(yrel);
    glm::vec3 const dir = glm::normalize(this->orientation_ * this->pos_);
    //logger.error(fmt::format("dir is '{}, {}, {}'", dir.x, dir.y, dir.z));

    int x, y;
    SDL_GetMouseState(&x, &y);

    bool const cpd = (y > 600.0f / 2.0f);
    auto const ytheta = glm::degrees(glm::angle(dir, Z_UNIT_VECTOR));
    logger.error(fmt::format("ytheta is {}, (x {}, y {}), xrel {}, yrel {}", ytheta, x, y, xrel, yrel));

    if (ytheta > 45.0f && (yrel > 0.0f) && cpd) {
      logger.error("DOWN LOCK");
      yangle = 0.0f;
    }
    if (ytheta > 45.0f && (yrel < 0.0f) && !cpd) {
      logger.error("UP LOCK");
      yangle = 0.0f;
    }
    auto const yaw = glm::angleAxis(xangle, glm::vec3{0.0f, 1.0f, 0.0f});
    auto const pitch = glm::angleAxis(yangle, -X_UNIT_VECTOR);
    this->orientation_ = yaw * pitch * this->orientation_;
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
