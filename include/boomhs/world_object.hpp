#pragma once
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <string>

namespace boomhs
{
class Camera;

class WorldObject
{
  Transform *transform_;
  glm::vec3 forward_, up_;

public:
  WorldObject(Transform&, glm::vec3 const&, glm::vec3 const&);
  MOVE_DEFAULT(WorldObject);
  COPY_DEFAULT(WorldObject);

  // fields

  // methods
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

  Transform& transform() { return *transform_; }
  Transform const& transform() const { return *transform_; }

  glm::quat const& orientation() const { return transform().rotation; }
  glm::vec3 const& world_position() const;

  auto forward() const { return forward_; }
  auto up() const { return up_; }

  WorldObject& move(glm::vec3 const&);

  void rotate_degrees(float const, glm::vec3 const&);
  void rotate_to_match_camera_rotation(Camera const&);

  void move_to(glm::vec3 const& pos) { transform().translation = pos; }
  void move_to(float const x, float const y, float const z) { move_to(glm::vec3{x, y, z}); }

  glm::mat4 model_matrix() const { return transform().model_matrix(); }
};

} // namespace boomhs
