#include <boomhs/frame.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;

namespace
{

auto
make_framestate(EngineState& es, ZoneState& zs, Camera const& camera,
                ViewSettings const& view_settings, Frustum const& frustum,
                glm::vec3 const& camera_world_pos, CameraMode const mode,
                bool const ortho_squeeze = true)
{
  glm::mat4 proj, view;

  auto& target = camera.get_target();
  auto& target_pos = target.transform().translation;

  switch (mode) {
    case CameraMode::Ortho:
      proj = camera.ortho.compute_projectionmatrix(ortho_squeeze, view_settings, frustum);
      view = camera.ortho.compute_viewmatrix(target_pos);
      break;
    case CameraMode::FPS:
      proj = camera.fps.compute_projectionmatrix(view_settings, frustum);
      view = camera.fps.compute_viewmatrix(camera.eye_forward());
      break;
    case CameraMode::ThirdPerson:
      proj = camera.arcball.compute_projectionmatrix(view_settings, frustum);
      view = camera.arcball.compute_viewmatrix(camera_world_pos);
      break;
    case CameraMode::FREE_FLOATING:
    case CameraMode::MAX:
      std::abort();
      break;
  }
  CameraFrameState cfs{camera_world_pos, proj, view, frustum, mode};
  return FrameState{es, zs, MOVE(cfs)};
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameState
FrameState
FrameState::from_camera_withposition(EngineState& es, ZoneState& zs, Camera const& camera,
                                     ViewSettings const& vs, Frustum const& frustum,
                                     glm::vec3 const& custom_camera_pos)
{
  auto const cmode = camera.mode();
  return make_framestate(es, zs, camera, vs, frustum, custom_camera_pos, cmode);
}

FrameState
FrameState::from_camera_with_mode(EngineState& es, ZoneState& zs, Camera const& camera, CameraMode const mode, ViewSettings const& vs, Frustum const& frustum)
{
  auto const camera_world_pos = camera.world_position();
  return make_framestate(es, zs, camera, vs, frustum, camera_world_pos, mode);
}

FrameState
FrameState::from_camera_for_2dui_overlay(EngineState& es, ZoneState& zs, Camera const& camera, ViewSettings const& vs, Frustum const& frustum)
{
  auto const camera_world_pos = camera.world_position();
  return make_framestate(es, zs, camera, vs, frustum, camera_world_pos, CameraMode::Ortho, false);
}

FrameState
FrameState::from_camera(EngineState& es, ZoneState& zs, Camera const& camera, ViewSettings const& vs, Frustum const& frustum)
{
  auto const pos = camera.world_position();
  return from_camera_withposition(es, zs, camera, vs, frustum, pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameState
FrameState::FrameState(EngineState& e, ZoneState& z, CameraFrameState&& cfs)
    : cfs_(MOVE(cfs))
    , es(e)
    , zs(z)
{
}

glm::vec3
FrameState::camera_world_position() const
{
  return cfs_.camera_world_position;
}

Frustum const&
FrameState::frustum() const
{
  return cfs_.frustum;
}

glm::mat4
FrameState::camera_matrix() const
{
  return projection_matrix() * view_matrix();
}

glm::mat4
FrameState::projection_matrix() const
{
  return cfs_.projection_matrix;
}

glm::mat4
FrameState::view_matrix() const
{
  return cfs_.view_matrix;
}

CameraMode
FrameState::camera_mode() const
{
  return cfs_.mode;
}

} // namespace boomhs
