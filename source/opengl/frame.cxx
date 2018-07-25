#include <opengl/frame.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>

using namespace boomhs;

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////
// FrameMatrices
FrameMatrices
FrameMatrices::from_camera_withposition(Camera const& camera, glm::vec3 const& custom_camera_pos)
{
  auto const  mode        = camera.mode();
  auto const& perspective = camera.perspective();
  auto const& ortho       = camera.ortho();

  auto const proj = Camera::compute_projectionmatrix(mode, perspective, ortho);

  auto const& target       = camera.get_target().translation;
  auto const  position_xyz = custom_camera_pos;
  auto const& up           = camera.eye_up();
  auto const& fps_center   = camera.world_forward() + target;

  auto const view = Camera::compute_viewmatrix(mode, position_xyz, target, up, fps_center);

  return FrameMatrices{custom_camera_pos, proj, view};
}

FrameMatrices
FrameMatrices::from_camera(Camera const& camera)
{
  return from_camera_withposition(camera, camera.world_position());
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
