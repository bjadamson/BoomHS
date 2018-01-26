#pragma once
#include <boomhs/components.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/types.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{
class Camera;

class WorldObject
{
  EnttLookup ent_lookup_;
  glm::vec3 forward_, up_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(WorldObject);
  explicit WorldObject(EnttLookup const& plookup, glm::vec3 const& forward, glm::vec3 const& up)
    : ent_lookup_(plookup)
    , forward_(forward)
    , up_(up)
  {
  }

  auto const&
  transform() const
  {
    return ent_lookup_.lookup<Transform>();
  }

  auto&
  transform()
  {
    return ent_lookup_.lookup<Transform>();
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
    return transform().rotation;
  }

  glm::vec3 const&
  world_position() const
  {
    return transform().translation;
  }

  auto&
  move(float const s, glm::vec3 const& dir)
  {
    transform().translation += (s * dir);
    return *this;
  }

  void
  rotate(float const, glm::vec3 const&);

  void
  rotate_to_match_camera_rotation(Camera const&);

  auto
  tilemap_position() const
  {
    auto const& pos = transform().translation;

    // Truncate the floating point values to get tilemap position
    auto const trunc = [](float const v) { return abs(v); };
    return glm::vec3{trunc(pos.x), trunc(pos.y), trunc(pos.z)};
  }

  void
  move_to(glm::vec3 const& pos)
  {
    transform().translation = pos;
  }

  void
  move_to(TilePosition const& pos)
  {
    move_to(glm::vec3{pos.x, pos.y, pos.z});
  }

  void
  set_eid(std::uint32_t const eid)
  {
    ent_lookup_.set_eid(eid);
  }

  glm::mat4
  model_matrix() const { return transform().model_matrix(); }
};

} // ns boomhs
