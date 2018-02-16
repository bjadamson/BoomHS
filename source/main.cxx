#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/components.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tiledata.hpp>
#include <boomhs/tiledata_algorithms.hpp>
#include <boomhs/ui.hpp>
#include <boomhs/zone.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <boost/algorithm/string.hpp>
#include <stlw/optional.hpp>

#include <cassert>
#include <cstdlib>
#include <string>

using namespace boomhs;
using namespace opengl;
using namespace window;
using stlw::Logger;

namespace boomhs
{

void
move_betweentiledatas_ifonstairs(TiledataState &tds, ZoneManager &zm)
{
  auto &lstate = zm.active().level_state;
  auto const& leveldata = lstate.level_data;

  auto &player = lstate.player;
  player.transform().translation.y = 0.5f;
  auto const wp = player.world_position();
  {
    auto const [w, h] = leveldata.dimensions();
    assert(wp.x < w);
    assert(wp.y < h);
  }
  auto const& tiledata = leveldata.tiledata();
  auto const& tile = tiledata.data(wp.x, wp.z);
  if (!tile.is_stair()) {
    return;
  }

  auto const move_player_through_stairs = [&tile, &tds, &zm](StairInfo const& stair) {
    {
      int const current = zm.active_zone();
      int const newlevel = current + (tile.is_stair_up() ? 1 : -1);
      assert(newlevel < zm.num_zones());
      zm.make_zone_active(newlevel, tds);
    }

    // now that the zone has changed, all references through zm are pointing to old level.
    // use active()
    auto &zs = zm.active();
    auto &lstate = zs.level_state;

    auto &camera = lstate.camera;
    auto &player = lstate.player;
    auto &registry = zs.registry;

    auto const spos = stair.exit_position;
    player.move_to(spos.x, player.world_position().y, spos.y);
    player.rotate_to_match_camera_rotation(camera);

    tds.recompute = true;
  };

  // BEGIN
  player.move_to(wp.x, wp.y, wp.z);
  auto const tp = TilePosition::from_floats_truncated(wp.x, wp.z);

  // lookup stairs in the registry
  auto &registry = zm.active().registry;
  auto const stair_eids = find_stairs(registry);
  assert(!stair_eids.empty());

  for (auto const& eid : stair_eids) {
    auto const& stair = registry.get<StairInfo>(eid);
    if (stair.tile_position == tp) {
      move_player_through_stairs(stair);

      // TODO: not just jump first stair we find
      break;
    }
  }
}

bool
wiggle_outofbounds(RiverInfo const& rinfo, RiverWiggle const& wiggle)
{
  auto const& pos = wiggle.position;
  return ANYOF(
      pos.x > rinfo.right,
      pos.x < rinfo.left,
      pos.y < rinfo.bottom,
      pos.y > rinfo.top);
}

void
reset_position(RiverInfo &rinfo, RiverWiggle &wiggle)
{
  auto const& tp = rinfo.origin;

  // reset the wiggle's position, then move it to the hidden cache
  wiggle.position = glm::vec2{tp.x, tp.y};
}

void
update_riverwiggles(LevelData &level_data, FrameTime const& ft)
{
  auto const update_river = [&ft](auto &rinfo)
  {
    for (auto &wiggle : rinfo.wiggles) {
      auto &pos = wiggle.position;
      pos += wiggle.direction * wiggle.speed * ft.delta;

      if (wiggle_outofbounds(rinfo, wiggle)) {
        reset_position(rinfo, wiggle);
      }
    }
  };
  for (auto &rinfo : level_data.rivers()) {
    update_river(rinfo);
  }
}

void
game_loop(EngineState &es, ZoneManager &zm, SDLWindow &window, FrameTime const& ft)
{
  auto &logger = es.logger;
  auto &tiledata_state = es.tiledata_state;
  auto &zs = zm.active();
  auto &lstate = zs.level_state;

  move_betweentiledatas_ifonstairs(tiledata_state, zm);
  update_riverwiggles(lstate.level_data, ft);

  /////////////////////////
  auto &leveldata = lstate.level_data;

  auto &player = lstate.player;
  auto &registry = zs.registry;
  /////////////////////////

  // compute tiledata
  if (tiledata_state.recompute) {
    LOG_INFO("Updating tiledata\n");

    update_visible_tiles(leveldata.tiledata(), player, tiledata_state.reveal);

    // We don't need to recompute the tiledata, we just did.
    tiledata_state.recompute = false;
  }

  // action begins here
  render::clear_screen(lstate.background);

  RenderState rstate{es, zs};
  if (es.draw_entities) {
    render::draw_entities(rstate);
  }
  if (es.draw_terrain) {
    render::draw_terrain(rstate);
  }
  if (tiledata_state.draw_tiledata) {
    render::draw_tiledata(rstate, tiledata_state, ft);
    render::draw_rivers(rstate, ft);
  }
  if (tiledata_state.show_grid_lines) {
    render::draw_tilegrid(rstate, tiledata_state);
  }
  if (tiledata_state.show_neighbortile_arrows) {
    auto const& wp = player.world_position();
    auto const tpos = TilePosition::from_floats_truncated(wp.x, wp.z);
    render::draw_arrow_abovetile_and_neighbors(rstate, tpos);
  }
  if (es.show_global_axis) {
    render::draw_global_axis(rstate, registry);
  }
  if (es.show_local_axis) {
    render::draw_local_axis(rstate, registry, player.world_position());
  }

  // if checks happen inside fn
  render::conditionally_draw_player_vectors(rstate, player);
  if (es.ui_state.draw_ui) {
    draw_ui(es, zm, window, registry);
  }
}

} // ns boomhs

namespace
{

struct Engine
{
  SDLWindow window;
  SDLControllers controllers;
  std::vector<entt::DefaultRegistry> registries = {};

  Engine() = delete;
  explicit Engine(SDLWindow &&w, SDLControllers &&c)
    : window(MOVE(w))
    , controllers(MOVE(c))
  {
    registries.resize(50);
  }

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPY_AND_NO_MOVE(Engine);

  auto dimensions() const { return window.get_dimensions(); }
};

void
loop(Engine &engine, GameState &state, FrameTime const& ft)
{
  auto &logger = state.engine_state.logger;
  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  SDL_Event event;
  boomhs::IO::process(state, event, engine.controllers, ft);

  ZoneManager zm{state.zone_states};
  boomhs::game_loop(state.engine_state, zm, engine.window, ft);

  // Render Imgui UI
  ImGui::Render();

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(engine.window.raw());
}

void
timed_game_loop(Engine &engine, GameState &state)
{
  window::Clock clock;
  window::FrameCounter counter;

  auto &logger = state.engine_state.logger;
  while (!state.engine_state.quit) {
    auto const ft = clock.frame_time();
    loop(engine, state, ft);
    clock.update(logger);
    counter.update(logger, clock);
  }
}

auto
make_init_gamestate(stlw::Logger &logger, ImGuiIO &imgui, window::Dimensions const &dimensions,
    ZoneStates &&zs)
{
  // Initialize opengl
  render::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  EngineState es{logger, imgui, dimensions};
  return GameState{MOVE(es), MOVE(zs)};
}

stlw::result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  auto &registries = engine.registries;
  DO_TRY(ZoneStates zss, LevelAssembler::assemble_levels(logger, registries));

  auto &imgui = ImGui::GetIO();
  auto state = make_init_gamestate(logger, imgui, engine.dimensions(), MOVE(zss));
  timed_game_loop(engine, state);
  LOG_TRACE("game loop finished.");
  return stlw::empty_type{};
}

} // ns anon

using WindowResult = stlw::result<SDLWindow, std::string>;
WindowResult
make_window(Logger &logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window::sdl_library::init());

  LOG_DEBUG("Instantiating window instance.");
  return window::sdl_library::make_window(fullscreen, height, width);
}

int
main(int argc, char *argv[])
{
  Logger logger = stlw::log_factory::make_default_logger("main logger");
  auto const on_error = [&logger](auto const &error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  LOG_DEBUG("Creating window ...");
  bool constexpr FULLSCREEN = false;
  DO_TRY_OR_ELSE_RETURN(auto window, make_window(logger, FULLSCREEN, 1024, 768),
                        on_error);
  DO_TRY_OR_ELSE_RETURN(auto controller, SDLControllers::find_attached_controllers(logger), on_error);
  Engine engine{MOVE(window), MOVE(controller)};

  LOG_DEBUG("Starting game loop");
  DO_TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
