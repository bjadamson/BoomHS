#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/world_object.hpp>

#include <extlibs/fmt.hpp>
#include <common/algorithm.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;
using namespace boomhs::math::constants;

namespace boomhs
{

WorldObject::WorldObject(Transform& tr, glm::vec3 const& forward, glm::vec3 const& up)
    : transform_(&tr)
    , forward_(forward)
    , up_(up)
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
  glm::vec3 camera_wo_fwd = -camera.world_forward();
  camera_wo_fwd.y         = 0;
  camera_wo_fwd           = glm::normalize(camera_wo_fwd);

  glm::vec3 wo_fwd = world_forward();
  wo_fwd.y         = 0;
  wo_fwd           = glm::normalize(wo_fwd);

  float const angle = math::angle_between_vectors(camera_wo_fwd, wo_fwd, ZERO);
  glm::quat const new_rotation = math::rotation_between_vectors(camera_wo_fwd, wo_fwd);

  auto& t    = transform();
  t.rotation = new_rotation * t.rotation;
}

} // namespace boomhs
