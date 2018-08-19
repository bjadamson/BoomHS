#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/world_object.hpp>

#include <extlibs/fmt.hpp>
#include <common/algorithm.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;

namespace boomhs
{

WorldObject::WorldObject(EntityID const eid, EntityRegistry& r, glm::vec3 const& forward,
                         glm::vec3 const& up)
    : eid_(eid)
    , registry_(&r)
    , forward_(forward)
    , up_(up)
{
  registry_->assign<Transform>(eid_);
}

glm::vec3
WorldObject::eye_forward() const
{
  return forward_ * orientation();
}

glm::vec3
WorldObject::eye_up() const
{
  return up_ * orientation();
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
WorldObject::rotate_degrees(float const angle, EulerAxis const axis)
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
  glm::vec3 camera_wo_fwd = -camera.eye_forward();
  camera_wo_fwd.y         = 0;
  camera_wo_fwd           = glm::normalize(camera_wo_fwd);

  glm::vec3 wo_fwd = eye_forward();
  wo_fwd.y         = 0;
  wo_fwd           = glm::normalize(wo_fwd);

  glm::quat const rot_between = math::rotation_between_vectors(camera_wo_fwd, wo_fwd);
  auto& t = transform();
  t.set_rotation(rot_between * t.rotation_quat());
}

} // namespace boomhs
