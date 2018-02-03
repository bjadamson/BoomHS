#include <boomhs/world_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/zone.hpp>

#include <stlw/format.hpp>
#include <stlw/math.hpp>

namespace boomhs
{

std::string
WorldObject::display() const
{
  return fmt::sprintf(
      "world_pos: '%s'\neye_forward: '%s'\nworld_forward: '%s'\n"
      "world_right: '%s'\nworld_up: '%s'\nquat: '%s'\n",
      glm::to_string(world_position()),
      glm::to_string(eye_forward()),
      glm::to_string(world_forward()),
      glm::to_string(world_right()),
      glm::to_string(world_up()),
      glm::to_string(orientation())
      );
}

void
WorldObject::rotate(float const angle, glm::vec3 const& axis)
{
  auto &t = transform();
  t.rotation = glm::angleAxis(glm::radians(angle), axis) * t.rotation;
}

void
WorldObject::rotate_to_match_camera_rotation(Camera const& camera)
{
  // camera "forward" is actually reverse due to -Z being +Z in eyespace.
  glm::vec3 eyespace_fwd = -camera.world_forward();
  eyespace_fwd.y = 0;
  eyespace_fwd = glm::normalize(eyespace_fwd);

  glm::vec3 player_fwd = world_forward();
  player_fwd.y = 0;
  player_fwd = glm::normalize(player_fwd);

  float const angle = stlw::math::angle_between_vectors(eyespace_fwd, player_fwd, glm::zero<glm::vec3>());
  glm::quat new_rotation = stlw::math::rotation_between_vectors(eyespace_fwd, player_fwd);

  auto &t = transform();
  t.rotation = new_rotation * t.rotation;
}

} // ns boomhs
