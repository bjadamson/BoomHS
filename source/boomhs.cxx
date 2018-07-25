#include <boomhs/audio.hpp>
#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/entity_renderer.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/state.hpp>
#include <boomhs/sun.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water.hpp>
#include <boomhs/water_renderer.hpp>

#include <opengl/gpu.hpp>
#include <opengl/heightmap.hpp>
#include <opengl/texture.hpp>

#include <extlibs/sdl.hpp>
#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <stlw/math.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>

#include <extlibs/fastnoise.hpp>
#include <extlibs/imgui.hpp>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

void
update_playaudio(stlw::Logger& logger, LevelData& ldata, EntityRegistry& registry,
                 WaterAudioSystem& audio)
{
  auto&      player = ldata.player;
  auto const eids = find_all_entities_with_component<WaterInfo, Transform, AABoundingBox>(registry);
  for (auto const eid : eids) {
    auto& water_bbox = registry.get<AABoundingBox>(eid);
    auto& w_tr       = registry.get<Transform>(eid);
    auto& p_tr       = player.transform();

    auto const& player_bbox = player.bounding_box();
    bool const  collides = collision::bbox_intersects(logger, p_tr, player_bbox, w_tr, water_bbox);

    if (collides) {
      audio.play_inwater_sound(logger);

      if (audio.is_playing_watersound()) {
        LOG_TRACE("PLAYING IN-WATER SOUND");
      }
    }
    else {
      audio.stop_inwater_sound(logger);
    }
  }
}

void
update_playerpos(stlw::Logger& logger, LevelData& ldata, FrameTime const& ft)
{
  // Lookup the player height from the terrain at the player's X, Z world-coordinates.
  auto&       player        = ldata.player;
  auto&       player_pos    = player.transform().translation;
  float const player_height = ldata.terrain.get_height(logger, player_pos.x, player_pos.z);
  auto const& player_bbox   = player.bounding_box();
  player_pos.y              = player_height + (player_bbox.dimensions().y / 2.0f);
}

void
update_nearbytargets(LevelData& ldata, EntityRegistry& registry, FrameTime const& ft)
{
  auto const player = find_player(registry);
  assert(registry.has<Transform>(player));
  auto const& ptransform = registry.get<Transform>(player);

  auto const enemies = find_enemies(registry);
  using pair_t       = std::pair<float, EntityID>;
  std::vector<pair_t> pairs;
  for (auto const eid : enemies) {
    if (!registry.get<IsVisible>(eid).value) {
      continue;
    }
    auto const& etransform = registry.get<Transform>(eid);
    float const distance   = glm::distance(ptransform.translation, etransform.translation);
    pairs.emplace_back(std::make_pair(distance, eid));
  }

  auto const sort_fn = [](auto const& a, auto const& b) { return a.first < b.first; };
  std::sort(pairs.begin(), pairs.end(), sort_fn);

  auto&      nbt        = ldata.nearby_targets;
  auto const selected_o = nbt.selected();

  nbt.clear();
  for (auto const& it : pairs) {
    nbt.add_target(it.second);
  }

  if (selected_o) {
    nbt.set_selected(*selected_o);
  }
}

void
update_orbital_bodies(EngineState& es, LevelData& ldata, glm::mat4 const& view_matrix,
                      glm::mat4 const& proj_matrix, EntityRegistry& registry, FrameTime const& ft)
{
  auto& logger      = es.logger;
  auto& directional = ldata.global_light.directional;

  // Must recalculate zs and registry, possibly changed since call to move_between()
  auto const update_orbitals = [&](auto const eid, bool const first) {
    auto& transform = registry.get<Transform>(eid);
    auto& orbital   = registry.get<OrbitalBody>(eid);
    auto& pos       = transform.translation;

    auto constexpr SLOWDOWN_FACTOR = 5.0f;
    auto const  time               = ft.since_start_seconds() / SLOWDOWN_FACTOR;
    float const cos_time           = std::cos(time + orbital.offset);
    float const sin_time           = std::sin(time + orbital.offset);

    pos.x = orbital.x_radius * cos_time;
    pos.y = orbital.y_radius * sin_time;
    pos.z = orbital.z_radius * sin_time;

    // TODO: HACK
    if (first) {
      auto const orbital_to_origin = glm::normalize(-pos);
      directional.direction        = orbital_to_origin;

      auto const mvp  = (proj_matrix * view_matrix) * transform.model_matrix();
      auto const clip = mvp * glm::vec4{pos, 1.0f};
      auto const ndc  = glm::vec3{clip.x, clip.y, clip.z} / clip.w;

      auto const wx = ((ndc.x + 1.0f) / 2.0f); // + 256.0;
      auto const wy = ((ndc.y + 1.0f) / 2.0f); // + 192.0;

      glm::vec2 const wpos{wx, wy};
      directional.screenspace_pos = wpos;

      LOG_ERROR_SPRINTF("player (world) pos: %s, clip: %s, ndc pos: %s, wpos: %s",
                        glm::to_string(pos), glm::to_string(clip), glm::to_string(ndc),
                        glm::to_string(wpos));

      // assert(ndc.x <= 1.0f && ndc.x >= -1.0f);
      // assert(ndc.y <= 1.0f && ndc.y >= -1.0f);
      // assert(ndc.z <= 1.0f && ndc.z >= -1.0f);
    }
  };

  auto const eids = find_orbital_bodies(registry);
  if (es.ui_state.debug.update_orbital_bodies) {
    bool first = true;
    for (auto const eid : eids) {
      update_orbitals(eid, first);
      first = false;
    }
  }
}

inline auto
find_torches(EntityRegistry& registry)
{
  std::vector<EntityID> torches;
  auto                  view = registry.view<Torch>();
  for (auto const eid : view) {
    assert(registry.has<Transform>(eid));
    torches.emplace_back(eid);
  }
  return torches;
}

void
update_torchflicker(LevelData const& ldata, EntityRegistry& registry, stlw::float_generator& rng,
                    FrameTime const& ft)
{
  auto const update_torch = [&](auto const eid) {
    auto& pointlight = registry.get<PointLight>(eid);

    auto const v       = std::sin(ft.since_start_millis() * M_PI);
    auto&      flicker = registry.get<LightFlicker>(eid);
    auto&      light   = pointlight.light;
    light.diffuse      = Color::lerp(flicker.colors[0], flicker.colors[1], v);
    light.specular     = light.diffuse;

    auto& item            = registry.get<Item>(eid);
    auto& torch_transform = registry.get<Transform>(eid);
    if (item.is_pickedup) {
      // Player has picked up the torch, make it follow player around
      auto const& player     = ldata.player;
      auto const& player_pos = player.world_position();

      torch_transform.translation = player_pos;

      // Move the light above the player's head
      torch_transform.translation.y = 1.0f;
    }

    auto const torch_pos   = torch_transform.translation;
    auto&      attenuation = pointlight.attenuation;

    auto const attenuate = [&rng](float& value, float const gen_range, float const base_value) {
      value += rng.gen_float_range(-gen_range, gen_range);

      auto const clamp = gen_range * 2.0f;
      value            = glm::clamp(value, base_value - clamp, base_value + clamp);
    };

    static float constexpr CONSTANT = 0.1f;
    // attenuate(attenuation.constant, CONSTANT, torch.default_attenuation.constant);

    // static float constexpr LINEAR = 0.015f;
    // attenuate(attenuation.linear, LINEAR, torch.default_attenuation.linear);

    // static float constexpr QUADRATIC = LINEAR * LINEAR;
    // attenuate(attenuation.quadratic, QUADRATIC, torch.default_attenuation.quadratic);

    static float constexpr SPEED_DELTA = 0.24f;
    attenuate(flicker.current_speed, SPEED_DELTA, flicker.base_speed);
  };
  auto const torches = find_torches(registry);
  for (auto const eid : torches) {
    update_torch(eid);
  }
}

void
update_visible_entities(LevelManager& lm, EntityRegistry& registry)
{
  auto& zs       = lm.active();
  auto& ldata    = zs.level_data;
  auto& tilegrid = ldata.tilegrid();
  auto& player   = ldata.player;

  for (auto const eid : registry.view<NPCData>()) {
    auto& isv = registry.get<IsVisible>(eid);

    // Convert to tile position, match tile visibility.
    auto&              transform = registry.get<Transform>(eid);
    auto const&        pos       = transform.translation;
    TilePosition const tpos      = TilePosition::from_floats_truncated(pos.x, pos.z);

    auto const& tile = tilegrid.data(tpos);
    isv.value        = tile.is_visible(registry);
  }
}

} // namespace

namespace boomhs
{

Result<GameState, std::string>
create_gamestate(Engine& engine, EngineState& engine_state, Camera& camera)
{
  ZoneStates zss =
      TRY_MOVEOUT(LevelAssembler::assemble_levels(engine_state.logger, engine.registries));
  GameState state{engine_state, LevelManager{MOVE(zss)}};

  auto& es     = state.engine_state;
  auto& logger = es.logger;

  auto const player_eid = find_player(engine.registries[0]);
  auto&      transform  = engine.registries[0].get<Transform>(player_eid);
  camera.set_target(transform);

  auto& lm        = state.level_manager;
  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  {
    TerrainConfig const tc;
    auto&               sp     = sps.ref_sp(tc.shader_name);
    auto&               ttable = gfx_state.texture_table;

    char const* HEIGHTMAP_NAME = "Area0-HM";
    auto const  heightmap =
        TRY_MOVEOUT(opengl::heightmap::load_fromtable(logger, ttable, HEIGHTMAP_NAME));

    TerrainGridConfig const tgc;
    auto                    tg = terrain::generate_grid(logger, tgc, tc, heightmap, sp);
    auto&                   ld = zs.level_data;
    ld.terrain                 = MOVE(tg);
  }
  {
    auto test_r = rexpaint::RexImage::load("assets/test.xp");
    if (!test_r) {
      LOG_ERROR_SPRINTF("%s", test_r.unwrapErrMove());
      std::abort();
    }
    auto test = test_r.expect_moveout("loading text.xp");
    test.flatten();
    auto save = rexpaint::RexImage::save(test, "assets/test.xp");
    if (!save) {
      LOG_ERROR_SPRINTF("%s", save.unwrapErrMove().to_string());
      std::abort();
    }
  }

  return OK_MOVE(state);
}

void
place_water(stlw::Logger& logger, ZoneState& zs, ShaderProgram& sp, glm::vec2 const& pos)
{
  auto& registry  = zs.registry;
  auto& gfx_state = zs.gfx_state;
  auto& gpu_state = gfx_state.gpu_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  auto const eid = registry.create();

  LOG_TRACE("Placing Water");
  auto* p = &registry.assign<WaterInfo>(eid);
  *p      = WaterFactory::make_default(logger, sps, ttable);

  auto& wi    = registry.get<WaterInfo>(eid);
  wi.position = pos;

  size_t constexpr num_vertexes = 64;
  glm::vec2 constexpr dimensions{20};
  auto const data = WaterFactory::generate_water_data(logger, dimensions, num_vertexes);
  {
    BufferFlags const flags{true, false, false, true};
    auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
    auto              dinfo  = gpu::copy_gpu(logger, sp.va(), buffer);

    auto& entities = gpu_state.entities;

    entities.add(eid, MOVE(dinfo));
    wi.dinfo = &entities.lookup(logger, eid);
  }
  {
    auto& bbox_entities = gpu_state.entity_boundingboxes;

    auto& bbox = registry.assign<AABoundingBox>(eid);
    bbox.min   = glm::vec3{-0.5, -0.2, -0.5};
    bbox.max   = glm::vec3{0.5f, 0.2, 0.5};

    CubeVertices const cv{bbox.min, bbox.max};

    auto& sp    = sps.ref_sp("wireframe");
    auto  dinfo = opengl::gpu::copy_cube_wireframevertexonly_gpu(logger, cv, sp.va());
    bbox_entities.add(eid, MOVE(dinfo));
  }

  registry.assign<Selectable>(eid);
  registry.assign<ShaderName>(eid);
  registry.assign<IsVisible>(eid).value = true;

  auto& tr         = registry.assign<Transform>(eid);
  tr.translation.x = dimensions.x / 2.0f;
  tr.translation.z = dimensions.y / 2.0f;

  tr.scale.x = dimensions.x;
  tr.scale.z = dimensions.y;
}

void
init(GameState& state)
{
  auto& logger = state.engine_state.logger;

  for (auto& zs : state.level_manager) {
    auto& sps = zs.gfx_state.sps;

    auto& water_sp = draw_water_options_to_shader(GameGraphicsMode::Basic, sps);
    place_water(logger, zs, water_sp, glm::vec2{0.0f, 0.0f});
    place_water(logger, zs, water_sp, glm::vec2{20.0f, 20.0f});
  }
}

void
ingame_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
            WaterAudioSystem& water_audio, FrameTime const& ft)
{
  auto& es = state.engine_state;
  es.time.update(ft.since_start_seconds());

  auto& logger   = es.logger;
  auto& lm       = state.level_manager;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  auto& ldata  = zs.level_data;
  auto& skybox = ldata.skybox;

  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  // TODO: Move out into state somewhere.
  auto const make_skybox_renderer = [&]() {
    auto&              skybox_sp = sps.ref_sp("skybox");
    glm::vec3 const    vmin{-0.5f};
    glm::vec3 const    vmax{0.5f};
    CubeVertices const cv{vmin, vmax};
    DrawInfo           dinfo    = opengl::gpu::copy_cubetexture_gpu(logger, cv, skybox_sp.va());
    auto&              day_ti   = *ttable.find("building_skybox");
    auto&              night_ti = *ttable.find("night_skybox");
    return SkyboxRenderer{logger, MOVE(dinfo), day_ti, night_ti, skybox_sp};
  };

  auto const make_basic_water_renderer = [&]() {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = draw_water_options_to_shader(GameGraphicsMode::Basic, sps);
    return BasicWaterRenderer{logger, diff, normal, sp};
  };

  auto const make_medium_water_renderer = [&]() {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = draw_water_options_to_shader(GameGraphicsMode::Medium, sps);
    return MediumWaterRenderer{logger, diff, normal, sp};
  };

  auto const&      dim = es.dimensions;
  ScreenSize const screen_size{dim.w, dim.h};
  auto const       make_advanced_water_renderer = [&]() {
    auto& ti     = *ttable.find("water-diffuse");
    auto& dudv   = *ttable.find("water-dudv");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = draw_water_options_to_shader(GameGraphicsMode::Advanced, sps);
    return AdvancedWaterRenderer{logger, screen_size, sp, ti, dudv, normal};
  };

  auto const make_black_water_renderer = [&]() {
    auto& sp = sps.ref_sp("silhoutte_black");
    return BlackWaterRenderer{logger, sp};
  };

  auto const make_black_terrain_renderer = [&]() {
    auto& sp = sps.ref_sp("silhoutte_black");
    return BlackTerrainRenderer{sp};
  };

  // TODO: move these (they are static for convenience testing)
  static SkyboxRenderer skybox_renderer         = make_skybox_renderer();
  static auto           basic_water_renderer    = make_basic_water_renderer();
  static auto           medium_water_renderer   = make_medium_water_renderer();
  static auto           advanced_water_renderer = make_advanced_water_renderer();
  static auto           black_water_renderer    = make_black_water_renderer();

  static auto basic_terrain_renderer  = BasicTerrainRenderer{};
  static auto black_terrain_renderer  = make_black_terrain_renderer();
  static auto default_entity_renderer = EntityRenderer{};
  static auto black_entity_renderer   = BlackEntityRenderer{};

  auto const fmatrices = FrameMatrices::from_camera(camera);
  FrameState fstate{fmatrices, es, zs};

  DrawState   ds;
  RenderState rstate{fstate, ds};

  {
    // Update the world
    update_playaudio(logger, ldata, registry, water_audio);
    update_playerpos(logger, ldata, ft);
    update_nearbytargets(ldata, registry, ft);

    update_orbital_bodies(es, ldata, fstate.view_matrix(), fstate.projection_matrix(), registry,
                          ft);
    skybox.update(ft);

    update_visible_entities(lm, registry);
    update_torchflicker(ldata, registry, rng, ft);
  }

  auto const& water_buffer = es.ui_state.debug.buffers.water;
  auto const  water_type = static_cast<GameGraphicsMode>(water_buffer.selected_water_graphicsmode);
  bool const  draw_water = water_buffer.draw;

  auto const& graphics_settings      = es.graphics_settings;
  bool const  graphics_mode_advanced = GameGraphicsMode::Advanced == graphics_settings.mode;
  bool const  draw_water_advanced    = draw_water && graphics_mode_advanced;

  auto&                   sunshaft_sp = sps.ref_sp("sunshaft");
  static SunshaftRenderer sunshaft_renderer{logger, screen_size, sunshaft_sp};

  auto const draw_scene = [&](bool const black_silhoutte) {
    auto const draw_advanced = [&](auto& terrain_renderer, auto& entity_renderer) {
      advanced_water_renderer.render_reflection(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
      advanced_water_renderer.render_refraction(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
    };
    if (draw_water && draw_water_advanced && !black_silhoutte) {
      // Render the scene to the refraction and reflection FBOs
      draw_advanced(basic_terrain_renderer, default_entity_renderer);
    }

    // render scene
    if (es.draw_skybox) {
      auto const clear_color = black_silhoutte ? LOC::BLACK : ldata.fog.color;
      render::clear_screen(clear_color);

      if (!black_silhoutte) {
        skybox_renderer.render(rstate, ds, ft);
      }
    }

    // The water must be drawn BEFORE rendering the scene the last time, otherwise it shows up
    // ontop of the ingame UI nearby target indicators.
    if (draw_water) {
      if (black_silhoutte) {
        black_water_renderer.render_water(rstate, ds, lm, camera, ft);
      }
      else {
        if (GameGraphicsMode::Basic == water_type) {
          basic_water_renderer.render_water(rstate, ds, lm, camera, ft);
        }
        else if (GameGraphicsMode::Medium == water_type) {
          medium_water_renderer.render_water(rstate, ds, lm, camera, ft);
        }
        else if (GameGraphicsMode::Advanced == water_type) {
          advanced_water_renderer.render_water(rstate, ds, lm, camera, ft);
        }
        else {
          std::abort();
        }
      }
    }

    // Render the scene with no culling (setting it zero disables culling mathematically)
    glm::vec4 const NOCULL_VECTOR{0, 0, 0, 0};
    if (es.draw_terrain) {
      auto const draw_basic = [&](auto& terrain_renderer, auto& entity_renderer) {
        terrain_renderer.render(rstate, registry, ft, NOCULL_VECTOR);
      };
      if (black_silhoutte) {
        draw_basic(black_terrain_renderer, black_entity_renderer);
      }
      else {
        draw_basic(basic_terrain_renderer, default_entity_renderer);
      }
    }
    if (es.draw_entities) {
      if (black_silhoutte) {
        black_entity_renderer.render(rstate, rng, ft);
      }
      else {
        default_entity_renderer.render(rstate, rng, ft);
      }
    }
    render::render_scene(rstate, lm, rng, ft, NOCULL_VECTOR);
  };

  auto const draw_scene_normal_render = [&]() { draw_scene(false); };

  auto const render_scene_with_sunshafts = [&]() {
    // draw scene with black silhouttes into the sunshaft FBO.
    sunshaft_renderer.with_sunshaft_fbo(logger, [&]() { draw_scene(true); });

    // draw the scene (normal render) to the screen
    draw_scene_normal_render();

    // With additive blending enabled, render the FBO ontop of the previously rendered scene to
    // obtain the sunglare effect.
    ENABLE_ADDITIVE_BLENDING_UNTIL_SCOPE_EXIT();
    sunshaft_renderer.render(rstate, ds, lm, camera, ft);
  };

  if (!graphics_settings.disable_sunshafts) {
    render_scene_with_sunshafts();
  }
  else {
    draw_scene_normal_render();
  }

  /*
  {
    glm::vec2 const pos{0.15f, -0.5f};
    glm::vec2 const scale{0.50f, 0.50f};

    render::draw_fbo_testwindow(rstate, pos, scale, sunshaft_renderer.ti());
  }
  */
  /*
  {
    glm::vec2 const pos{-0.5f, -0.5f};
    glm::vec2 const scale{0.25f, 0.25f};

    render::draw_fbo_testwindow(fstate, pos, scale, dudv);
  }
  */

  auto& ui_state = es.ui_state;
  if (ui_state.draw_ingame_ui) {
    ui_ingame::draw(es, lm);
  }

  if (ui_state.draw_debug_ui) {
    auto& lm = state.level_manager;
    ui_debug::draw(es, lm, skybox_renderer, water_audio, engine.window, camera, ds, ft);
  }
}

void
game_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
          FrameTime const& ft)
{
  auto& es = state.engine_state;
  auto& io = es.imgui;

  static auto audio_r     = WaterAudioSystem::create();
  static auto water_audio = audio_r.expect_moveout("WAS");

  if (es.main_menu.show) {
    // Enable keyboard shortcuts
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // clear the screen before rending the main menu1
    render::clear_screen(LOC::BLACK);

    auto const& size = engine.dimensions();
    main_menu::draw(es, ImVec2(size.w, size.h), water_audio);
  }
  else {
    // Disable keyboard shortcuts
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    IO::process(state, engine.controllers, camera, ft);
    ingame_loop(engine, state, rng, camera, water_audio, ft);
  }
}

} // namespace boomhs
