#include <opengl/debug_renderer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/components.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/player.hpp>

#include <boomhs/random.hpp>

#include <boomhs/clock.hpp>
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

  auto& player = find_player(registry);
  if (es.show_global_axis) {
    render::draw_axis(rstate, math::constants::ZERO);
  }

  Transform camera_transform;
  camera_transform.translation = camera.world_position();
  auto const model = camera_transform.model_matrix();

  if (es.draw_view_frustum) {
    render::draw_frustum(rstate, camera.frustum_ref(), model);
  }

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
