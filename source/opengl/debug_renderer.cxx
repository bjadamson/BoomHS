#include <opengl/debug_renderer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/components.hpp>
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
DebugRenderer::render_scene(RenderState& rstate, LevelManager& lm,
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

  render::draw_frustrum(rstate, fstate.view_matrix(), fstate.projection_matrix());

  {
    auto const  eid = find_player(registry);
    auto const& inv = registry.get<Player>(eid).inventory;
    if (inv.is_open()) {
      render::draw_inventory_overlay(rstate);
    }
  }

  // if checks happen inside fn
  render::conditionally_draw_player_vectors(rstate, player);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BlackSceneRenderer
void
BlackSceneRenderer::render_scene(RenderState&, LevelManager&, RNG&,
               FrameTime const&)
{
}

} // namespace opengl
