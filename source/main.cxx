#include <boomhs/components.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/ui.hpp>
#include <boomhs/level_manager.hpp>

#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>

#include <fastnoise/fastnoise.hpp>

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
move_betweentilegrids_ifonstairs(TiledataState &tds, LevelManager &lm)
{
  auto &ldata = lm.active().level_data;

  auto &player = ldata.player;
  player.transform().translation.y = 0.5f;
  auto const wp = player.world_position();
  {
    auto const [w, h] = ldata.dimensions();
    assert(wp.x < w);
    assert(wp.z < h);
  }
  auto const& tilegrid = ldata.tilegrid();
  auto const& tile = tilegrid.data(wp.x, wp.z);
  if (tile.type == TileType::TELEPORTER) {
    int const current = lm.active_zone();
    int const newlevel = current == 0 ? 1 : 0;
    assert(newlevel < lm.num_levels());
    lm.make_active(newlevel, tds);
    std::cerr << "setting level to: '" << newlevel << "'\n";


    // now that the zone has changed, all references through lm are pointing to old level.
    // use active()
    auto &zs = lm.active();
    auto &ldata = zs.level_data;

    auto &player = ldata.player;
    auto &registry = zs.registry;

    player.move_to(10, player.world_position().y, 10);
    return;
  }
  if (!tile.is_stair()) {
    return;
  }

  auto const move_player_through_stairs = [&tile, &tds, &lm](StairInfo const& stair) {
    {
      int const current = lm.active_zone();
      int const newlevel = current + (tile.is_stair_up() ? 1 : -1);
      assert(newlevel < lm.num_levels());
      lm.make_active(newlevel, tds);
    }

    // now that the zone has changed, all references through lm are pointing to old level.
    // use active()
    auto &zs = lm.active();
    auto &ldata = zs.level_data;

    auto &camera = ldata.camera;
    auto &player = ldata.player;
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
  auto &registry = lm.active().registry;
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

void
update_nearbytargets(LevelData &ldata, EntityRegistry &registry, FrameTime const& ft)
{
  ldata.nearby_targets.clear();

  auto const player = find_player(registry);
  assert(registry.has<Transform>(player));
  auto const& ptransform = registry.get<Transform>(player);

  auto const enemies = find_enemies(registry);
  using pair_t = std::pair<float, EntityID>;
  std::vector<pair_t> pairs;
  for (auto const eid : enemies) {
    if (!registry.get<IsVisible>(eid).value) {
      continue;
    }
    auto const& etransform = registry.get<Transform>(eid);
    float const distance = glm::distance(ptransform.translation, etransform.translation);
    pairs.emplace_back(std::make_pair(distance, eid));
  }

  auto const sort_fn = [](auto const& a, auto const& b) {
    return a.first < b.first;
  };
  std::sort(pairs.begin(), pairs.end(), sort_fn);

  for (auto const& it : pairs) {
    ldata.nearby_targets.add_target(it.second);
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
      pos += wiggle.direction * wiggle.speed * ft.delta_millis();

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
update_torchflicker(LevelData const& ldata, EntityRegistry &registry, stlw::float_generator &rng,
    FrameTime const& ft)
{
  auto const eid = ldata.torch_eid();

  auto &pointlight = registry.get<PointLight>(eid);

  auto const v = std::sin(ft.since_start_millis() * M_PI);
  auto &flicker = registry.get<LightFlicker>(eid);
  pointlight.light.diffuse = Color::lerp(flicker.colors[0], flicker.colors[1], v);

  auto &torch = registry.get<Torch>(eid);
  auto &torch_transform = registry.get<Transform>(eid);
  if (torch.is_pickedup) {
    // Player has picked up the torch, make it follow player around
    auto const& player = ldata.player;
    auto const& player_pos = player.world_position();

    torch_transform.translation = player_pos;

    // Move the light above the player's head
    torch_transform.translation.y = 1.0f;
  }

  auto const torch_pos = torch_transform.translation;
  auto &attenuation = pointlight.attenuation;

  auto const attenuate = [&rng](float &value, float const gen_range, float const base_value)
  {
    value += rng.gen_float_range(-gen_range, gen_range);

    auto const clamp = gen_range * 2.0f;
    value = glm::clamp(value, base_value - clamp, base_value + clamp);
  };

  static float constexpr CONSTANT = 0.1f;
  //attenuate(attenuation.constant, CONSTANT, torch.default_attenuation.constant);

  //static float constexpr LINEAR = 0.015f;
  //attenuate(attenuation.linear, LINEAR, torch.default_attenuation.linear);

  //static float constexpr QUADRATIC = LINEAR * LINEAR;
  //attenuate(attenuation.quadratic, QUADRATIC, torch.default_attenuation.quadratic);

  static float constexpr SPEED_DELTA = 0.24f;
  attenuate(flicker.current_speed, SPEED_DELTA, flicker.base_speed);
}

void
update_visible_entities(LevelManager &lm, EntityRegistry &registry)
{
  auto &zs = lm.active();
  auto &ldata = zs.level_data;
  auto &tilegrid = ldata.tilegrid();
  auto &player = ldata.player;

  for (auto const eid : registry.view<NPCData>()) {
    auto &isv = registry.get<IsVisible>(eid);

    // Convert to tile position, match tile visibility.
    auto &transform = registry.get<Transform>(eid);
    auto const& pos = transform.translation;
    TilePosition const tpos = TilePosition::from_floats_truncated(pos.x, pos.z);

    auto const& tile = tilegrid.data(tpos);
    isv.value = tile.is_visible(registry);
  }
}

void
game_loop(EngineState &es, LevelManager &lm, SDLWindow &window, stlw::float_generator &rng,
    FrameTime const& ft)
{
  auto &logger = es.logger;
  auto &tilegrid_state = es.tilegrid_state;

  // Update the world
  {
    auto &zs = lm.active();
    auto &registry = zs.registry;
    move_betweentilegrids_ifonstairs(tilegrid_state, lm);
  }

  // Must recalculate zs and registry, possibly changed since call to move_between()
  auto &zs = lm.active();
  auto &registry = zs.registry;
  auto &ldata = zs.level_data;
  auto &player = ldata.player;
  {
    update_nearbytargets(ldata, registry, ft);
    move_riverwiggles(ldata, ft);

    if (tilegrid_state.recompute) {
      // compute tilegrid
      LOG_INFO("Updating tilegrid\n");

      update_visible_tiles(ldata.tilegrid(), player, tilegrid_state.reveal);

      // We don't need to recompute the tilegrid, we just did.
      tilegrid_state.recompute = false;
    }

    // river wiggles get updated every frame
    update_visible_riverwiggles(ldata, player, tilegrid_state.reveal);

    update_visible_entities(lm, registry);
    update_torchflicker(ldata, registry, rng, ft);
  }
  {
    // rendering code
    render::clear_screen(ldata.background);

    RenderState rstate{es, zs};
    if (es.draw_entities) {
      render::draw_entities(rstate, rng, ft);
    }
    if (es.draw_terrain) {
      render::draw_terrain(rstate);
    }
    if (tilegrid_state.draw_tilegrid) {
      render::draw_tilegrid(rstate, tilegrid_state, ft);
      render::draw_rivers(rstate, ft);
    }

    render::draw_stars(rstate, ft);
    render::draw_targetreticle(rstate, ft);

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
    if (es.ui_state.draw_ingame_ui) {
      draw_ingame_ui();
    }
    if (es.ui_state.draw_debug_ui) {
      draw_debug_ui(es, lm, window, registry);
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
  std::vector<EntityRegistry> registries = {};

  Engine() = delete;
  explicit Engine(SDLWindow &&w, SDLControllers &&c)
    : window(MOVE(w))
    , controllers(MOVE(c))
  {
    registries.resize(50);
  }

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPYMOVE(Engine);

  auto dimensions() const { return window.get_dimensions(); }
};

void
loop(Engine &engine, GameState &state, stlw::float_generator &rng, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &lm = state.level_manager;

  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(engine.window.raw());

  {
    SDL_Event event;
    boomhs::IO::process(state, event, engine.controllers, ft);
  }
  boomhs::game_loop(es, lm, engine.window, rng, ft);

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
  stlw::float_generator rng;

  auto &logger = state.engine_state.logger;
  while (!state.engine_state.quit) {
    auto const ft = clock.frame_time();
    loop(engine, state, rng, ft);
    clock.update();
    counter.update(logger, clock);
  }
}

Result<stlw::empty_type, std::string>
start(stlw::Logger &logger, Engine &engine)
{
  // Initialize GUI library
  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  auto &registries = engine.registries;
  ZoneStates zss = TRY_MOVEOUT(LevelAssembler::assemble_levels(logger, registries));

  // Initialize opengl
  auto const dimensions = engine.dimensions();
  render::init(dimensions);

  // Configure Imgui
  auto &imgui = ImGui::GetIO();
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  auto test_r = rexpaint::RexImage::load("assets/test.xp");
  if (!test_r) {
    std::cerr << test_r << "\n";
    std::abort();
  }
  auto test = test_r.expect_moveout("loading text.xp");
  test.flatten();
  auto save = rexpaint::RexImage::save(test, "assets/test.xp");
  if (!save) {
    std::cerr << save << "\n";
    std::abort();
  }

  // Construct game state
  EngineState es{logger, imgui, dimensions};
  GameState state{MOVE(es), LevelManager{MOVE(zss)}};

  // Start game in a timed loop
  timed_game_loop(engine, state);

  // Game has finished
  LOG_TRACE("game loop finished.");
  return Ok(stlw::empty_type{});
}

} // ns anon

using WindowResult = Result<SDLWindow, std::string>;
WindowResult
make_window(stlw::Logger &logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  auto _ =  window::sdl_library::init();

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
  TRY_OR_ELSE_RETURN(auto window, make_window(logger, FULLSCREEN, 1024, 768),
                        on_error);
  TRY_OR_ELSE_RETURN(auto controller, SDLControllers::find_attached_controllers(logger), on_error);
  Engine engine{MOVE(window), MOVE(controller)};

  LOG_DEBUG("Starting game loop");
  TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
