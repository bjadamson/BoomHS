#pragma once
#include <boomhs/camera.hpp>

#include <extlibs/glm.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{
class Camera;
class EngineState;
class ZoneState;
} // namespace boomhs

namespace opengl
{

struct CameraFrameState
{
  glm::vec3 const camera_world_position;
  glm::mat4 const projection;
  glm::mat4 const view;

  static CameraFrameState from_camera_withposition(boomhs::Camera const&, glm::vec3 const&);
  static CameraFrameState from_camera_with_mode(boomhs::Camera const&, boomhs::CameraMode);

  static CameraFrameState from_camera(boomhs::Camera const&);
};

struct DrawState
{
  size_t num_vertices;
  size_t num_drawcalls;

  DrawState();

  std::string to_string() const;
};

class FrameState
{
  CameraFrameState const cstate_;

public:
  NO_COPY_OR_MOVE(FrameState);
  FrameState(CameraFrameState const&, boomhs::EngineState&, boomhs::ZoneState&);

  boomhs::EngineState& es;
  boomhs::ZoneState&   zs;

  glm::vec3 camera_world_position() const;
  glm::mat4 camera_matrix() const;
  glm::mat4 projection_matrix() const;
  glm::mat4 view_matrix() const;
};

} // namespace opengl
