#include <boomhs/camera.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/world_object.hpp>

#include <extlibs/fmt.hpp>
#include <stlw/math.hpp>

namespace boomhs
{

WorldObject::WorldObject(EnttLookup const& plookup, glm::vec3 const& forward, glm::vec3 const& up)
    : ent_lookup_(plookup)
    , forward_(forward)
    , up_(up)
    , speed_(460)
{
}

WorldObject&
WorldObject::move(glm::vec3 const& delta)
{
  transform().translation += delta;
  return *this;
}

std::string
WorldObject::display() const
{
  return fmt::sprintf("world_pos: '%s'\neye_forward: '%s'\nworld_forward: '%s'\n"
                      "world_right: '%s'\nworld_up: '%s'\nquat: '%s'\n",
                      glm::to_string(world_position()), glm::to_string(eye_forward()),
                      glm::to_string(world_forward()), glm::to_string(world_right()),
                      glm::to_string(world_up()), glm::to_string(orientation()));
}

glm::vec3 const&
WorldObject::world_position() const
{
  return transform().translation;
}

void
WorldObject::rotate_degrees(float const angle, glm::vec3 const& axis)
{
  auto& t = transform();
  t.rotate_degrees(angle, axis);
}

void
WorldObject::rotate_to_match_camera_rotation(Camera const& camera)
{
  // General procedure:
  //
  // Calculate eye forward and player forward vectors, zeroing out the Y components and normlaizing
  // so they can be compared on the XZ plane.
  //
  // Calculate angle between eye and player forward vectors, and rotate the world object the
  // calculated amount.
  //
  // The result is the object is facing the same direction as the camera on the XZ plane.
  //
  // NOTE: Camera "forward" is actually reverse due to -Z being +Z in eyespace.
  glm::vec3 eyespace_fwd = -camera.world_forward();
  eyespace_fwd.y         = 0;
  eyespace_fwd           = glm::normalize(eyespace_fwd);

  glm::vec3 wo_fwd = world_forward();
  wo_fwd.y         = 0;
  wo_fwd           = glm::normalize(wo_fwd);

  float const angle =
      stlw::math::angle_between_vectors(eyespace_fwd, wo_fwd, glm::zero<glm::vec3>());
  glm::quat const new_rotation = stlw::math::rotation_between_vectors(eyespace_fwd, wo_fwd);

  auto& t    = transform();
  t.rotation = new_rotation * t.rotation;
}

TilePosition
WorldObject::tile_position() const
{
  auto const& pos = transform().translation;
  assert(pos.x >= 0.0f);
  assert(pos.z >= 0.0f);
  return TilePosition::from_floats_truncated(pos.x, pos.z);
}

} // namespace boomhs
