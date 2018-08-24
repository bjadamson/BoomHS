#pragma once
#include <common/type_macros.hpp>
#include <extlibs/glm.hpp>

#include <array>

namespace boomhs
{
class Camera;
class EngineState;
class ZoneState;

enum class CameraMode
{
  ThirdPerson = 0,
  Ortho,
  FPS,
  FREE_FLOATING,
  MAX
};

struct CameraModes
{
  using ModeNamePair = std::pair<CameraMode, char const*>;
  CameraModes()      = delete;

  static std::array<ModeNamePair, static_cast<size_t>(CameraMode::MAX)> constexpr CAMERA_MODES = {
      {{CameraMode::Ortho, "Ortho"},
       {CameraMode::ThirdPerson, "ThirdPerson"},
       {CameraMode::FPS, "FPS"},
       {CameraMode::FREE_FLOATING, "Free Floating"}}};

  static std::vector<std::string> string_list();
};

struct CameraFrameState
{
  glm::vec3 const camera_world_position;
  glm::mat4 const projection_matrix;
  glm::mat4 const view_matrix;

  CameraMode const mode;
};

class FrameState
{
  CameraFrameState const cfs_;

public:
  FrameState(EngineState&, ZoneState&, CameraFrameState&&);
  NO_COPY_OR_MOVE(FrameState);

  EngineState& es;
  ZoneState&   zs;

  glm::vec3 camera_world_position() const;
  glm::mat4 camera_matrix() const;
  glm::mat4 projection_matrix() const;
  glm::mat4 view_matrix() const;
  CameraMode camera_mode() const;

  static FrameState from_camera_withposition(EngineState&, ZoneState&, Camera const&,
                                             glm::vec3 const&);
  static FrameState from_camera_with_mode(EngineState&, ZoneState&, Camera const&, CameraMode);

  static FrameState from_camera(EngineState&, ZoneState&, Camera const&);
};

} // namespace boomhs
