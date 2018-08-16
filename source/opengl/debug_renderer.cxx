#include <opengl/debug_renderer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/components.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>

#include <boomhs/random.hpp>

#include <window/timer.hpp>
#include <extlibs/glm.hpp>

using namespace boomhs;
using namespace window;

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// DebugRenderer
void
DebugRenderer::render_scene(RenderState& rstate, LevelManager& lm, Camera& camera,
                                   RNG& rng, FrameTime const& ft)
{
  auto& fstate = rstate.fs;
  auto& es     = fstate.es;
  auto& logger = es.logger;

  auto& zs       = fstate.zs;
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;

  if (es.show_grid_lines) {
    render::draw_grid_lines(rstate);
  }

  auto const player_eid = find_player(registry);
  auto const& player = registry.get<Player>(player_eid);
  if (es.show_global_axis) {
    render::draw_global_axis(rstate);
  }
  if (es.show_local_axis) {
    render::draw_local_axis(rstate, player.world_position());
  }

  Transform camera_transform;
  camera_transform.translation = camera.world_position();
  auto const model = camera_transform.model_matrix();
  render::draw_frustum(rstate, camera.frustum_ref(), model);

  // if checks happen inside fn
  render::conditionally_draw_player_vectors(rstate, player);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BlackSceneRenderer
void
BlackSceneRenderer::render_scene(RenderState&, LevelManager&, Camera&, RNG&,
               FrameTime const&)
{
}

} // namespace opengl
