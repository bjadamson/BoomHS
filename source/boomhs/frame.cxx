#include <boomhs/frame.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>

using namespace boomhs;

namespace
{

auto
from_camera_common(Camera const& camera, glm::vec3 const& pos, CameraMode const mode)
{
  auto const proj = camera.compute_projectionmatrix();
  auto const view = camera.compute_viewmatrix(pos);
  return CameraFrameState{pos, proj, view, mode};
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////
// CameraFrameState
CameraFrameState
CameraFrameState::from_camera_withposition(Camera const& camera, glm::vec3 const& custom_camera_pos)
{
  auto const mode = camera.mode();
  return from_camera_common(camera, custom_camera_pos, mode);
}

CameraFrameState
CameraFrameState::from_camera_with_mode(Camera const& camera, CameraMode const mode)
{
  auto const pos = camera.world_position();
  return from_camera_common(camera, pos, mode);
}

CameraFrameState
CameraFrameState::from_camera(Camera const& camera)
{
  auto const pos = camera.world_position();
  return from_camera_withposition(camera, pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameState
FrameState::FrameState(CameraFrameState const& cstate, EngineState& e, ZoneState& z)
    : cstate_(cstate)
    , es(e)
    , zs(z)
{
}

glm::vec3
FrameState::camera_world_position() const
{
  return cstate_.camera_world_position;
}

glm::mat4
FrameState::camera_matrix() const
{
  auto const proj = projection_matrix();
  auto const view = view_matrix();
  return proj * view;
}

glm::mat4
FrameState::projection_matrix() const
{
  return cstate_.projection;
}

glm::mat4
FrameState::view_matrix() const
{
  return cstate_.view;
}

CameraMode
FrameState::mode() const
{
  return cstate_.mode;
}

} // namespace boomhs
