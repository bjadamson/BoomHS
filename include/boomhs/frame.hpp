#pragma once
#include <extlibs/glm.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace boomhs
{
class Camera;
class EngineState;
class ZoneState;

struct FrameMatrices
{
  glm::vec3 const camera_world_position;
  glm::mat4 const projection;
  glm::mat4 const view;

  static FrameMatrices from_camera_withposition(Camera const&, glm::vec3 const&);

  static FrameMatrices from_camera(Camera const&);
};

struct DrawState
{
  size_t num_vertices;

  DrawState();

  std::string to_string() const;
};

class FrameState
{
  FrameMatrices const fmatrices_;

public:
  NO_COPYMOVE(FrameState);
  FrameState(FrameMatrices const&, EngineState&, ZoneState&);

  EngineState& es;
  ZoneState&   zs;

  glm::vec3 camera_world_position() const;
  glm::mat4 camera_matrix() const;
  glm::mat4 projection_matrix() const;
  glm::mat4 view_matrix() const;
};

} // namespace boomhs
