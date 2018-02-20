#include <boomhs/components.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/ui.hpp>
#include <boomhs/zone.hpp>

#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>

#include <entt/entt.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>

#include <cassert>
#include <cstdlib>
#include <string>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace boomhs
{

void
move_betweentilegrids_ifonstairs(TiledataState &tds, ZoneManager &zm)
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
  auto const& tilegrid = leveldata.tilegrid();
  auto const& tile = tilegrid.data(wp.x, wp.z);
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
move_riverwiggles(LevelData &level_data, FrameTime const& ft)
{
  auto const update_river = [&ft](auto &rinfo)
  {
    for (auto &wiggle : rinfo.wiggles) {
      auto &pos = wiggle.position;
      pos += wiggle.direction * wiggle.speed * ft.delta();

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
  auto &tilegrid_state = es.tilegrid_state;

  auto &zs = zm.active();
  auto &registry = zs.registry;

  auto &lstate = zs.level_state;
  auto &leveldata = lstate.level_data;
  auto &player = lstate.player;

  // Update the world
  {
    move_betweentilegrids_ifonstairs(tilegrid_state, zm);
    move_riverwiggles(lstate.level_data, ft);

    if (tilegrid_state.recompute) {
      // compute tilegrid
      LOG_INFO("Updating tilegrid\n");

      update_visible_tiles(leveldata.tilegrid(), player, tilegrid_state.reveal);

      // We don't need to recompute the tilegrid, we just did.
      tilegrid_state.recompute = false;
    }

    // river wiggles get updated every frame
    update_visible_riverwiggles(leveldata, player, tilegrid_state.reveal);
  }
  {
    // rendering code
    render::clear_screen(lstate.background);

    RenderState rstate{es, zs};
    if (es.draw_entities) {
      render::draw_entities(rstate);
    }
    if (es.draw_terrain) {
      render::draw_terrain(rstate);
    }
    if (tilegrid_state.draw_tilegrid) {
      render::draw_tilegrid(rstate, tilegrid_state, ft);
      render::draw_rivers(rstate, ft);
    }

    render::draw_stars(rstate, ft);

    if (tilegrid_state.show_grid_lines) {
      render::draw_tilegrid(rstate, tilegrid_state);
    }
    if (tilegrid_state.show_neighbortile_arrows) {
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
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &zm = state.zone_manager;

  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  {
    SDL_Event event;
    boomhs::IO::process(state, event, engine.controllers, ft);
  }
  boomhs::game_loop(es, zm, engine.window, ft);

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
    clock.update();
    counter.update(logger, clock);
  }
}

stlw::result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  auto &registries = engine.registries;
  DO_TRY(ZoneStates zss, LevelAssembler::assemble_levels(logger, registries));

  // Initialize opengl
  auto const dimensions = engine.dimensions();
  render::init(dimensions);

  // Configure Imgui
  auto &imgui = ImGui::GetIO();
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  auto test = rexpaint::RexImage::load("assets/test.xp");
  if (!test) {
    std::cerr << test.error() << "\n";
    std::abort();
  }
  (*test).flatten();
  auto save = rexpaint::RexImage::save(*test, "assets/test.xp");
  if (!save) {
    std::cerr << save.error() << "\n";
    std::abort();
  }

  // Construct game state
  EngineState es{logger, imgui, dimensions};
  GameState state{MOVE(es), ZoneManager{MOVE(zss)}};

  // Start game in a timed loop
  timed_game_loop(engine, state);

  // Game has finished
  LOG_TRACE("game loop finished.");
  return stlw::empty_type{};
}

} // ns anon

using WindowResult = stlw::result<SDLWindow, std::string>;
WindowResult
make_window(stlw::Logger &logger, bool const fullscreen, float const width, float const height)
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
  auto logger = stlw::log_factory::make_default_logger("main logger");
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
