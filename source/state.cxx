#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

namespace boomhs
{

RenderArgs
GameState::render_args()
{
  auto &logger = engine_state.logger;

  ZoneManager zm{zone_states};
  auto &active = zm.active();

  auto const& camera = active.camera;
  auto const& player = active.player;
  auto const& global_light = active.global_light;

  bool const draw_normals = engine_state.draw_normals;
  return RenderArgs{camera, player, logger, global_light, draw_normals};
}

} // ns boomhs
