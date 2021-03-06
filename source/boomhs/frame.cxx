#include <boomhs/camera.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/math.hpp>
#include <boomhs/viewport.hpp>

using namespace boomhs;

ScreenSize constexpr DEFAULT_ORTHO_VIEWSIZE{128, 128};

namespace
{

auto
make_framestate(EngineState& es, ZoneState& zs, Camera const& camera,
                ViewSettings const& view_settings, Frustum const& frustum,
                glm::vec3 const& camera_world_pos, CameraMode const mode)
{
  glm::mat4 proj, view;
  CameraMatrices cm;

  auto& target     = camera.get_target();
  auto& target_pos = target.transform().translation;

  switch (mode) {
  case CameraMode::Ortho: {
    cm = camera.ortho.calc_cm(frustum, DEFAULT_ORTHO_VIEWSIZE);
  } break;
  case CameraMode::Fullscreen_2DUI: {
    auto const vp = Viewport::from_frustum(frustum);
    cm          = camera.ortho.calc_cm(frustum, vp.size());
  } break;
  case CameraMode::FPS:
    cm = camera.fps.calc_cm(view_settings, frustum, camera.eye_forward());
    break;
  case CameraMode::ThirdPerson:
    cm = camera.arcball.calc_cm(view_settings, frustum, camera_world_pos);
    break;
  case CameraMode::FREE_FLOATING:
  case CameraMode::MAX:
    std::abort();
    break;
  }
  CameraFrameState cfs{camera_world_pos, cm, frustum, mode};
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
FrameState::from_camera_with_mode(EngineState& es, ZoneState& zs, Camera const& camera,
                                  CameraMode const mode, ViewSettings const& vs,
                                  Frustum const& frustum)
{
  auto const camera_world_pos = camera.position();
  return make_framestate(es, zs, camera, vs, frustum, camera_world_pos, mode);
}

FrameState
FrameState::from_camera_for_2dui_overlay(EngineState& es, ZoneState& zs, Camera const& camera,
                                         ViewSettings const& vs, Frustum const& frustum)
{
  auto const camera_world_pos = camera.position();
  return make_framestate(es, zs, camera, vs, frustum, camera_world_pos,
                         CameraMode::Fullscreen_2DUI);
}

FrameState
FrameState::from_camera(EngineState& es, ZoneState& zs, Camera const& cam, ViewSettings const& vs,
                        Frustum const& frustum)
{
  auto const pos = cam.position();
  return from_camera_withposition(es, zs, cam, vs, frustum, pos);
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

glm::mat4 const&
FrameState::projection_matrix() const
{
  return cfs_.camera_matrices.proj;
}

glm::mat4 const&
FrameState::view_matrix() const
{
  return cfs_.camera_matrices.view;
}

CameraMode
FrameState::camera_mode() const
{
  return cfs_.mode;
}

} // namespace boomhs
