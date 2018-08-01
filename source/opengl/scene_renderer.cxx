#include <opengl/scene_renderer.hpp>
#include <opengl/renderer.hpp>

#include <boomhs/components.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>

#include <stlw/random.hpp>

#include <window/timer.hpp>
#include <extlibs/glm.hpp>

using namespace boomhs;
using namespace window;

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// DefaultSceneRenderer
void
DefaultSceneRenderer::render_scene(RenderState& rstate, LevelManager& lm,
                                   stlw::float_generator& rng, FrameTime const& ft)
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
BlackSceneRenderer::render_scene(RenderState&, LevelManager&, stlw::float_generator&,
               FrameTime const&)
{
}

} // namespace opengl
