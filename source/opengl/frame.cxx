#include <opengl/frame.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto
from_camera_common(Camera const& camera, glm::vec3 const& pos, CameraMode const mode)
{
  auto const& perspective = camera.perspective();
  auto const& ortho       = camera.ortho();

  auto const proj = Camera::compute_projectionmatrix(mode, perspective, ortho);

  auto const& target       = camera.get_target().translation;
  auto const& up           = camera.eye_up();
  auto const& fps_center   = camera.world_forward() + target;

  auto const view = Camera::compute_viewmatrix(mode, pos, target, up, fps_center);
  return FrameMatrices{pos, proj, view};
}

} // namespace

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameMatrices
FrameMatrices
FrameMatrices::from_camera_withposition(Camera const& camera, glm::vec3 const& custom_camera_pos)
{
  auto const mode = camera.mode();
  return from_camera_common(camera, custom_camera_pos, mode);
}

FrameMatrices
FrameMatrices::from_camera_with_mode(Camera const& camera, CameraMode const mode)
{
  auto const pos = camera.world_position();
  return from_camera_common(camera, pos, mode);
}

FrameMatrices
FrameMatrices::from_camera(Camera const& camera)
{
  auto const pos = camera.world_position();
  return from_camera_withposition(camera, pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// DrawState
DrawState::DrawState()
{
  stlw::memzero(this, sizeof(DrawState));

  assert(0 == num_vertices);
  assert(0 == num_drawcalls);
}

std::string
DrawState::to_string() const
{
  return fmt::sprintf("{vertices: %lu, drawcalls: %lu}", num_vertices, num_drawcalls);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameState
FrameState::FrameState(FrameMatrices const& fmatrices, EngineState& e, ZoneState& z)
    : fmatrices_(fmatrices)
    , es(e)
    , zs(z)
{
}

glm::vec3
FrameState::camera_world_position() const
{
  return fmatrices_.camera_world_position;
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
  return fmatrices_.projection;
}

glm::mat4
FrameState::view_matrix() const
{
  return fmatrices_.view;
}

} // namespace opengl
