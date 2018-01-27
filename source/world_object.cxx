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
      "world_pos: '%s'\nforward_: '%s'\nforward_vector: '%s'\nright_vector: '%s'\nup_vector: '%s'\nquat: '%s'\n",
      glm::to_string(world_position()),
      glm::to_string(forward_),
      glm::to_string(forward_vector()),
      glm::to_string(right_vector()),
      glm::to_string(up_vector()),
      glm::to_string(orientation())
      );
}

void
WorldObject::rotate(float const angle, glm::vec3 const& axis)
{
  glm::quat const new_rotation{axis * glm::radians(angle)};
  auto &t = transform();
  t.rotation = new_rotation * t.rotation;
}

void
WorldObject::rotate_to_match_camera_rotation(Camera const& camera)
{
  glm::vec3 eyespace_fwd = camera.forward_vector();
  eyespace_fwd.y = 0;
  eyespace_fwd = glm::normalize(eyespace_fwd);

  glm::vec3 player_fwd = forward_vector();
  player_fwd.y = 0;


  using namespace stlw;
  float const angle = math::angle_between_vectors(eyespace_fwd, player_fwd, glm::zero<glm::vec3>());
  glm::quat new_rotation = math::rotation_between_vectors(eyespace_fwd, player_fwd);

  // TODO: I'm not sure why the extra 180 is requied currently. Without it, the player ends up
  // facing backwars (maybe -Z being FORWARD) is why? Either way, need to grok.
  new_rotation = new_rotation * glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

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
  auto const pos = wo.tilemap_position() + move_vec;
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
  auto const& new_tile = tilemap.data(pos + move_vec);
  if (!es.player_collision) {
    wo.move(move_vec, dt);
    ts.recompute = true;
  } else if (!new_tile.is_wall) {
    wo.move(move_vec, dt);
    ts.recompute = true;
  }
}

} // ns boomhs
