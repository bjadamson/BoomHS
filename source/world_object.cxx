#include <boomhs/world_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
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

///////////////////////////////////////////////////////////////////////////////////////////////////
void
move_ontilemap(GameState &state, glm::vec3 (WorldObject::*fn)() const, WorldObject &wo, double const dt)
{
  auto &es = state.engine_state;
  auto &ts = es.tilemap_state;

  ZoneManager zm{state.zone_states};
  auto const& tilemap = zm.active().tilemap;
  auto const [x, y, z] = tilemap.dimensions();

  auto const move_vec = (wo.*fn)();
  auto const pos = wo.tilemap_position() + (move_vec * dt * wo.speed());
  bool const x_outofbounds = pos.x > x || pos.x < 0;
  bool const y_outofbounds = pos.y > y || pos.y < 0;
  bool const z_outofbounds = pos.z > z || pos.z < 0;
  bool const out_of_bounds = x_outofbounds || y_outofbounds || z_outofbounds;

  if (out_of_bounds && es.mariolike_edges) {
    if (x_outofbounds) {
      auto const new_x = pos.x < 0 ? x : 0;
      wo.move_to(new_x, pos.y, pos.z);
    }
    else if (y_outofbounds) {
      auto const new_y = pos.y < 0 ? y : 0;
      wo.move_to(pos.x, new_y, pos.z);
    }
    else if (z_outofbounds) {
      auto const new_z = pos.z < 0 ? z : 0;
      wo.move_to(pos.x, pos.y, new_z);
    }
  } else if (out_of_bounds) {
    return;
  }
  auto const& new_tile = tilemap.data(pos);
  if (!es.player_collision) {
    wo.move_to(pos);
    ts.recompute = true;
  } else if (!new_tile.is_wall) {
    wo.move_to(pos);
    ts.recompute = true;
  }
}

} // ns boomhs
