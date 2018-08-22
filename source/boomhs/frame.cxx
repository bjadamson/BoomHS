#include <boomhs/frame.hpp>

#include <boomhs/camera.hpp>

using namespace boomhs;

namespace
{

auto
make_framestate(Camera const& camera, glm::vec3 const& camera_world_pos, CameraMode const mode)
{
  glm::mat4 proj, view;

  auto& target = camera.get_target();
  auto& target_pos = target.transform().translation;

  switch (mode) {
    case CameraMode::Ortho:
      proj = camera.ortho.compute_projectionmatrix();
      view = camera.ortho.compute_viewmatrix(target_pos);
      break;
    case CameraMode::FPS:
      proj = camera.fps.compute_projectionmatrix();
      view = camera.fps.compute_viewmatrix(camera.eye_forward());
      break;
    case CameraMode::ThirdPerson:
      proj = camera.arcball.compute_projectionmatrix();
      view = camera.arcball.compute_viewmatrix(camera_world_pos);
      break;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
      break;
  }
  return CameraFrameState{camera_world_pos, proj, view, mode};
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
  return make_framestate(camera, custom_camera_pos, mode);
}

CameraFrameState
CameraFrameState::from_camera_with_mode(Camera const& camera, CameraMode const mode)
{
  auto const camera_world_pos = camera.world_position();
  return make_framestate(camera, camera_world_pos, mode);
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
