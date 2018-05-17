#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water_fbos.hpp>

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
move_betweentilegrids_ifonstairs(stlw::Logger& logger, Camera& camera, TiledataState& tds,
                                 LevelManager& lm)
{
  auto& ldata = lm.active().level_data;

  auto& player                     = ldata.player;
  player.transform().translation.y = 0.5f;
  auto const wp                    = player.world_position();
  {
    auto const [w, h] = ldata.dimensions();
    assert(wp.x < w);
    assert(wp.z < h);
  }
  auto const& tilegrid = ldata.tilegrid();
  auto const& tile     = tilegrid.data(wp.x, wp.z);
  if (tile.type == TileType::TELEPORTER) {
    {
      int const current  = lm.active_zone();
      int const newlevel = current == 0 ? 1 : 0;
      assert(newlevel < lm.num_levels());
      LOG_TRACE_SPRINTF("setting level to: %i", newlevel);
      lm.make_active(newlevel, tds);
    }

    // now that the zone has changed, all references through lm are pointing to old level.
    // use active()
    auto& zs    = lm.active();
    auto& ldata = zs.level_data;

    auto& player   = ldata.player;
    auto& registry = zs.registry;

    player.move_to(10, player.world_position().y, 10);
    camera.set_target(player.transform());
    return;
  }
  if (!tile.is_stair()) {
    return;
  }

  auto const move_player_through_stairs = [&](StairInfo const& stair) {
    {
      int const current  = lm.active_zone();
      int const newlevel = current + (tile.is_stair_up() ? 1 : -1);
      assert(newlevel < lm.num_levels());
      lm.make_active(newlevel, tds);
    }

    // now that the zone has changed, all references through lm are pointing to old level.
    // use active()
    auto& zs    = lm.active();
    auto& ldata = zs.level_data;

    auto& player   = ldata.player;
    auto& registry = zs.registry;

    auto const spos = stair.exit_position;
    player.move_to(spos.x, player.world_position().y, spos.y);
    camera.set_target(player.transform());
    player.rotate_to_match_camera_rotation(camera);

    tds.recompute = true;
  };

  // BEGIN
  player.move_to(wp.x, wp.y, wp.z);
  auto const tp = TilePosition::from_floats_truncated(wp.x, wp.z);

  // lookup stairs in the registry
  auto&      registry   = lm.active().registry;
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

auto
rotate_around(glm::vec3 const& point_to_rotate, glm::vec3 const& rot_center,
              glm::mat4x4 const& rot_matrix)
{
  glm::mat4x4 const translate     = glm::translate(glm::mat4{}, rot_center);
  glm::mat4x4 const inv_translate = glm::translate(glm::mat4{}, -rot_center);

  // The idea:
  // 1) Translate the object to the center
  // 2) Make the rotation
  // 3) Translate the object back to its original location
  glm::mat4x4 const transform = translate * rot_matrix * inv_translate;
  auto const        pos       = transform * glm::vec4{point_to_rotate, 1.0f};
  return glm::vec3{pos.x, pos.y, pos.z};
}

void
update_orbital_bodies(EngineState& es, LevelData& ldata, EntityRegistry& registry,
                      FrameTime const& ft)
{
  auto& logger = es.logger;

  // Must recalculate zs and registry, possibly changed since call to move_between()
  auto const update_orbitals = [&](auto const eid, bool const first) {
    auto& transform = registry.get<Transform>(eid);
    auto& orbital   = registry.get<OrbitalBody>(eid);
    auto& pos       = transform.translation;

    auto const  time     = ft.since_start_seconds();
    float const cos_time = std::cos(time + orbital.offset);
    float const sin_time = std::sin(time + orbital.offset);

    pos.x = orbital.x_radius * cos_time;
    pos.y = orbital.y_radius * sin_time;
    pos.z = orbital.z_radius * sin_time;

    // TODO: HACK
    if (first) {
      auto& directional = ldata.global_light.directional;
      if (!directional.enabled) {
        return;
      }

      auto const orbital_to_origin = glm::normalize(-pos);
      directional.direction        = orbital_to_origin;
    }
  };

  if (es.ui_state.debug.update_orbital_bodies) {
    auto const eids  = find_orbital_bodies(registry);
    bool       first = true;
    for (auto const eid : eids) {
      update_orbitals(eid, first);
      first = false;
    }
  }
}

bool
wiggle_outofbounds(RiverInfo const& rinfo, RiverWiggle const& wiggle)
{
  auto const& pos = wiggle.position;
  return ANYOF(pos.x > rinfo.right, pos.x < rinfo.left, pos.y<rinfo.bottom, pos.y> rinfo.top);
}

void
reset_position(RiverInfo& rinfo, RiverWiggle& wiggle)
{
  auto const& tp = rinfo.origin;

  // reset the wiggle's position, then move it to the hidden cache
  wiggle.position = glm::vec2{tp.x, tp.y};
}

void
move_riverwiggles(LevelData& level_data, FrameTime const& ft)
{
  auto const update_river = [&ft](auto& rinfo) {
    for (auto& wiggle : rinfo.wiggles) {
      auto& pos = wiggle.position;
      pos += wiggle.direction * wiggle.speed * ft.delta_millis();

      if (wiggle_outofbounds(rinfo, wiggle)) {
        reset_position(rinfo, wiggle);
      }
    }
  };
  for (auto& rinfo : level_data.rivers()) {
    update_river(rinfo);
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
init(Engine& engine, EngineState& engine_state, Camera& camera)
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
    TerrainPieceConfig const tc;
    auto&                    sp     = sps.ref_sp(tc.shader_name);
    auto&                    ttable = gfx_state.texture_table;

    char const* HEIGHTMAP_NAME = "Area0-HM";
    auto const  heightmap = TRY(opengl::heightmap::load_fromtable(logger, ttable, HEIGHTMAP_NAME));
    auto*       ti        = ttable.find(tc.texture_name);
    assert(ti);

    TerrainGridConfig const tgc;
    auto                    tg = terrain::generate_grid(logger, tgc, tc, heightmap, sp, *ti);
    auto&                   ld = zs.level_data;
    ld.terrain().grid          = MOVE(tg);
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
render_scene(RenderState& rstate, LevelManager& lm, stlw::float_generator& rng, FrameTime const& ft,
             glm::vec4 const& cull_plane)
{
  auto& es    = rstate.es;
  auto& zs    = rstate.zs;
  auto& ldata = zs.level_data;

  if (es.draw_entities) {
    render::draw_entities(rstate, rng, ft);
  }

  auto& tilegrid_state = es.tilegrid_state;
  if (tilegrid_state.draw_tilegrid) {
    render::draw_tilegrid(rstate, tilegrid_state, ft);
    render::draw_rivers(rstate, ft);
  }

  auto& registry = zs.registry;
  if (es.draw_terrain) {
    render::draw_terrain(rstate, registry, ft, cull_plane);
  }

  render::draw_stars(rstate, ft);
  render::draw_targetreticle(rstate, ft);

  if (tilegrid_state.show_grid_lines) {
    render::draw_tilegrid(rstate, tilegrid_state);
  }

  auto& player = ldata.player;
  if (tilegrid_state.show_neighbortile_arrows) {
    auto const& wp   = player.world_position();
    auto const  tpos = TilePosition::from_floats_truncated(wp.x, wp.z);
    render::draw_arrow_abovetile_and_neighbors(rstate, tpos);
  }
  if (es.show_global_axis) {
    render::draw_global_axis(rstate);
  }
  if (es.show_local_axis) {
    render::draw_local_axis(rstate, player.world_position());
  }

  {
    auto const  eid = find_player(registry);
    auto const& inv = registry.get<PlayerData>(eid).inventory;
    if (inv.is_open()) {
      render::draw_inventory_overlay(rstate);
    }
  }

  // if checks happen inside fn
  render::conditionally_draw_player_vectors(rstate, player);
}

void
game_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
          FrameTime const& ft)
{
  auto& es = state.engine_state;
  es.time.update(ft.since_start_seconds());

  auto& logger         = es.logger;
  auto& tilegrid_state = es.tilegrid_state;
  auto& lm             = state.level_manager;

  // Update the world
  {
    auto& zs       = lm.active();
    auto& registry = zs.registry;
    move_betweentilegrids_ifonstairs(logger, camera, tilegrid_state, lm);
  }

  // Must recalculate zs and registry, possibly changed since call to move_between()
  auto& zs       = lm.active();
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;
  auto& player   = ldata.player;
  {
    update_nearbytargets(ldata, registry, ft);
    update_orbital_bodies(es, ldata, registry, ft);
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

  // TODO: Move out into state somewhere.
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;

  auto const&              dim = es.dimensions;
  ScreenSize const         screen_size{dim.w, dim.h};
  auto&                    ttable = gfx_state.texture_table;
  auto&                    ti     = *ttable.find("water-diffuse");
  auto&                    dudv   = *ttable.find("water-dudv");
  auto&                    normal = *ttable.find("water-normal");
  auto&                    sp     = sps.ref_sp("water");
  static WaterFrameBuffers waterfbos{logger, screen_size, sp, ti, dudv, normal};

  // Render the scene to the reflection FBO
  float constexpr CULL_CUTOFF_HEIGHT = 0.4f;
  glm::vec4 const ABOVE_VECTOR{0, -1, 0, CULL_CUTOFF_HEIGHT};
  glm::vec4 const BENEATH_VECTOR{0, 1, 0, -CULL_CUTOFF_HEIGHT};

  // Render the scene with no culling (setting it zero disables culling mathematically)
  glm::vec4 const NOCULL_VECTOR{0, 0, 0, 0};

  auto const& fog_color = ldata.fog.color;
  auto&       skybox_ti = *ttable.find("building_skybox");
  waterfbos.with_reflection_fbo(logger, [&]() {
    // Compute the camera position beneath the water for capturing the reflective image the camera
    // will see.
    //
    // By inverting the camera's Y position before computing the view matrices, we can render the
    // world as if the camera was beneath the water's surfac. This is how computing the reflection
    // texture works.
    glm::vec3 camera_pos = camera.world_position();
    camera_pos.y         = -camera_pos.y;

    auto const  reflect_rmatrices = RenderMatrices::from_camera_withposition(camera, camera_pos);
    RenderState rstate{reflect_rmatrices, es, zs};
    render::clear_screen(fog_color);

    render::clear_screen(ldata.fog.color);
    render::draw_skybox(rstate, skybox_ti, ft);
    render_scene(rstate, lm, rng, ft, ABOVE_VECTOR);
  });

  auto const  rmatrices = RenderMatrices::from_camera(camera);
  RenderState rstate{rmatrices, es, zs};
  waterfbos.with_refraction_fbo(logger, [&]() {
    render::clear_screen(fog_color);
    render::draw_skybox(rstate, skybox_ti, ft);
    render_scene(rstate, lm, rng, ft, BENEATH_VECTOR);
  });

  render::clear_screen(ldata.fog.color);
  render::draw_skybox(rstate, skybox_ti, ft);

  render_scene(rstate, lm, rng, ft, NOCULL_VECTOR);
  render::draw_water(rstate, registry, ft, ABOVE_VECTOR, waterfbos, camera.world_position());

  /*
  {
    glm::vec2 const pos{0.5f, -0.5f};
    glm::vec2 const scale{0.25f, 0.25f};

    render::draw_fbo_testwindow(rstate, pos, scale, normal);
  }
  {
    glm::vec2 const pos{-0.5f, -0.5f};
    glm::vec2 const scale{0.25f, 0.25f};

    render::draw_fbo_testwindow(rstate, pos, scale, dudv);
  }
  */

  auto& ui_state = es.ui_state;
  if (ui_state.draw_ingame_ui) {
    ui_ingame::draw(es, lm);
  }
}

} // namespace boomhs
