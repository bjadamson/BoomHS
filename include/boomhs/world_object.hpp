#pragma once
#include <boomhs/components.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/types.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{
class Camera;
struct GameState;

class WorldObject
{
  EnttLookup ent_lookup_;
  glm::vec3 forward_, up_;
  float speed_ = 240.0;

public:
  MOVE_CONSTRUCTIBLE_ONLY(WorldObject);
  explicit WorldObject(EnttLookup const& plookup, glm::vec3 const& forward, glm::vec3 const& up)
    : ent_lookup_(plookup)
    , forward_(forward)
    , up_(up)
  {
  }

  auto const& transform() const { return ent_lookup_.lookup<Transform>(); }
  auto& transform() { return ent_lookup_.lookup<Transform>(); }

  glm::vec3 eye_forward() const { return forward_; }
  glm::vec3 eye_up() const { return up_; }
  glm::vec3 eye_backward() const { return -eye_forward(); }

  glm::vec3 eye_left() const { return -eye_right(); }
  glm::vec3 eye_right() const { return glm::normalize(glm::cross(eye_forward(), eye_up())); }
  glm::vec3 eye_down() const { return -eye_up(); }

  glm::vec3 world_forward() const { return forward_ * orientation(); }
  glm::vec3 world_up() const { return up_ * orientation(); }
  glm::vec3 world_backward() const { return -world_forward(); }

  glm::vec3 world_left() const { return -world_right(); }
  glm::vec3 world_right() const { return glm::normalize(glm::cross(world_forward(), world_up())); }
  glm::vec3 world_down() const { return -world_up(); }

  std::string display() const;

  glm::quat const& orientation() const { return transform().rotation; }
  glm::vec3 const& world_position() const;

  auto speed() const { return speed_; }
  void set_speed(float const s) { speed_ = s; }

  auto&
  move(glm::vec3 const& dir, double const dt)
  {
    transform().translation += (speed() * dir) * static_cast<float>(dt);
    return *this;
  }

  void rotate(float const, glm::vec3 const&);

  void rotate_to_match_camera_rotation(Camera const&);

  TilePosition
  tilemap_position() const
  {
    auto const& pos = transform().translation;

    // Truncate the floating point values to get tilemap position
    auto const trunc = [](float const v) -> int { return abs(v); };
    return TilePosition{trunc(pos.x), trunc(pos.z)};
  }

  void move_to(glm::vec3 const& pos) { transform().translation = pos; }
  void move_to(float const x, float const y, float const z) { move_to(glm::vec3{x, y, z}); }

  void set_eid(std::uint32_t const eid) { ent_lookup_.set_eid(eid); }
  glm::mat4 model_matrix() const { return transform().model_matrix(); }
};

} // ns boomhs
