#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/math.hpp>
#include <boomhs/transform.hpp>

#include <common/type_macros.hpp>

#include <string>

namespace boomhs
{
class Camera;

class WorldObject
{
  EntityID        eid_      = EntityIDMAX;
  EntityRegistry* registry_ = nullptr;

  glm::vec3 forward_, up_;

public:
  WorldObject(EntityID, EntityRegistry&, glm::vec3 const&, glm::vec3 const&);
  NO_COPY(WorldObject);
  MOVE_DEFAULT(WorldObject);

  // The "local" vectors are the object's world vectors multiplied with the object's rotation.
  //
  // The direction the object face's, normalized.
  glm::vec3 eye_forward() const;
  glm::vec3 eye_up() const;
  glm::vec3 eye_backward() const { return -eye_forward(); }

  glm::vec3 eye_left() const { return -eye_right(); }
  glm::vec3 eye_right() const { return glm::normalize(glm::cross(eye_forward(), eye_up())); }
  glm::vec3 eye_down() const { return -eye_up(); }

  // The "world" vectors are the the 3 Axis unit vector (X_UNIT_VECTOR, Y_UNIT_VECTOR,
  // Z_UNIT_VECTOR) translated to the object's position.
  glm::vec3 world_forward() const { return forward_; }
  glm::vec3 world_up() const { return up_; }
  glm::vec3 world_backward() const { return -world_forward(); }

  glm::vec3 world_left() const { return -world_right(); }
  glm::vec3 world_right() const { return glm::normalize(glm::cross(world_forward(), world_up())); }
  glm::vec3 world_down() const { return -world_up(); }

  std::string display() const;

  Transform&       transform() { return registry_->get<Transform>(eid_); }
  Transform const& transform() const { return registry_->get<Transform>(eid_); }

  glm::quat        orientation() const { return transform().rotation; }
  glm::vec3 const& world_position() const;

  WorldObject& move(glm::vec3 const&);

  void rotate_degrees(float const, math::EulerAxis);
  void rotate_to_match_camera_rotation(Camera const&);

  void move_to(glm::vec3 const& pos) { transform().translation = pos; }
  void move_to(float const x, float const y, float const z) { move_to(glm::vec3{x, y, z}); }

  glm::mat4 model_matrix() const { return transform().model_matrix(); }
};

} // namespace boomhs
