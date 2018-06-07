#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
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
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water_fbos.hpp>

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

  auto&      player = ldata.player;
  auto const wp     = player.world_position();
  {
    auto const dim = ldata.dimensions();
    assert(wp.x < dim.x);
    assert(wp.z < dim.y);
  }
  /*
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
*/
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
    TerrainConfig const tc;
    auto&               sp     = sps.ref_sp(tc.shader_name);
    auto&               ttable = gfx_state.texture_table;

    char const* HEIGHTMAP_NAME = "Area0-HM";
    auto const  heightmap =
        TRY_MOVEOUT(opengl::heightmap::load_fromtable(logger, ttable, HEIGHTMAP_NAME));
    auto* ti = ttable.find(tc.texture_name);
    assert(ti);

    TerrainGridConfig const tgc;
    auto                    tg = terrain::generate_grid(logger, tgc, tc, heightmap, sp, *ti);
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
init_entities(stlw::Logger& logger, EntityRegistry& registry, ShaderPrograms& sps,
              TextureTable& ttable, GpuState& gpu_state)
{
  LOG_TRACE("Placing Water");
  auto const eid = registry.create();
  auto*      p   = &registry.assign<WaterInfo>(eid);
  *p             = WaterFactory::make_default(logger, sps, ttable);

  auto& wi = registry.get<WaterInfo>(eid);

  glm::vec2 const position{0, 0};
  size_t const    num_vertexes = 64;
  glm::vec2 const dimensions{20};
  auto const      data = WaterFactory::generate_water_data(logger, dimensions, num_vertexes);

  {
    BufferFlags const flags{true, false, false, true};
    auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
    auto&             sp     = sps.ref_sp("water");
    auto              dinfo  = gpu::copy_gpu(logger, sp.va(), buffer);

    auto& entities_o = gpu_state.entities;
    assert(entities_o);
    auto& entities = *entities_o;

    entities.add(eid, MOVE(dinfo));
    wi.dinfo = &entities.lookup(logger, eid);
  }
  {
    wi.position = position;
  }
  {
    auto& bbox_entities_o = gpu_state.entity_boundingboxes;
    assert(bbox_entities_o);
    auto& bbox_entities = *bbox_entities_o;

    auto& bbox = registry.assign<AABoundingBox>(eid);
    bbox.min   = glm::vec3{-0.5, -0.5, -0.5};
    bbox.max   = glm::vec3{0.5f, 0.5, 0.5};

    CubeVertices const cv{bbox.min, bbox.max};

    auto& sp    = sps.ref_sp("wireframe");
    auto  dinfo = opengl::gpu::copy_cube_wireframevertexonly_gpu(logger, cv, sp.va());
    bbox_entities.add(eid, MOVE(dinfo));
  }

  registry.assign<Selectable>(eid);
  registry.assign<ShaderName>(eid);
  registry.assign<IsVisible>(eid).value = true;
  registry.assign<Name>(eid).value      = "Water";

  auto& tr         = registry.assign<Transform>(eid);
  tr.translation.x = dimensions.x / 2.0f;
  tr.translation.z = dimensions.y / 2.0f;
  // tr.translation.y = -0.5f;

  tr.scale.x = dimensions.x;
  tr.scale.z = dimensions.y;
}

void
game_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
          FrameTime const& ft)
{
  auto& es = state.engine_state;
  es.time.update(ft.since_start_seconds());

  auto& logger = es.logger;
  auto& lm     = state.level_manager;

  // Update the world
  {
    auto& zs       = lm.active();
    auto& registry = zs.registry;

    auto& ldata  = zs.level_data;
    auto& player = ldata.player;

    // Lookup the player height from the terrain at the player's X, Z world-coordinates.
    auto&       player_pos    = player.transform().translation;
    float const player_height = ldata.terrain.get_height(logger, player_pos.x, player_pos.z);
    auto const& player_bbox   = player.bounding_box();
    player_pos.y              = player_height + (player_bbox.dimensions().y / 2.0f);

    auto const testAABBAABB_SIMD = [&logger](Transform const& at, AABoundingBox const& ab,
                                             Transform const& bt, AABoundingBox const& bb) {
      // SIMD optimized AABB-AABB test
      // Optimized by removing conditional branches
      auto const& ac = at.translation;
      auto const& bc = bt.translation;
      LOG_ERROR_SPRINTF("ac %s bc %s", glm::to_string(ac), glm::to_string(bc));

      auto const ah = ab.half_widths() * at.scale;
      auto const bh = bb.half_widths() * bt.scale;
      LOG_ERROR_SPRINTF("ah %s bh %s", glm::to_string(ah), glm::to_string(bh));

      bool x = std::fabs(ac.x - bc.x) <= (ah.x + bh.x);
      bool y = std::fabs(ac.y - bc.y) <= (ah.y + bh.y);
      bool z = std::fabs(ac.z - bc.z) <= (ah.z + bh.z);
      LOG_ERROR_SPRINTF("x %i y %i z %i", x, y, z);

      return x && y && z;
    };

    auto const weids =
        find_all_entities_with_component<WaterInfo, Transform, AABoundingBox>(registry);
    for (auto const eid : weids) {
      auto&      water_bbox = registry.get<AABoundingBox>(eid);
      auto&      w_tr       = registry.get<Transform>(eid);
      auto&      p_tr       = player.transform();
      bool const collides   = testAABBAABB_SIMD(p_tr, player_bbox, w_tr, water_bbox);
      if (collides) {
        LOG_ERROR_SPRINTF("PLAYER IN WATER");
      }
    }

    // auto const& dimensions = ldata.terrain.dimensions();
    // auto const  tp         = glm::vec3{::fmodf(player_pos.x, dimensions.x), player_pos.y,
    //::fmodf(player_pos.z, dimensions.y)};
    // LOG_ERROR_SPRINTF("player pos WP: %s TP: %s", glm::to_string(player_pos),
    // glm::to_string(tp));
    // move_betweentilegrids_ifonstairs(logger, camera, tilegrid_state, lm);
  }

  // Must recalculate zs and registry, possibly changed since call to move_between()
  auto& zs       = lm.active();
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;
  auto& player   = ldata.player;
  auto& skybox   = ldata.skybox;

  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  static bool once = false;
  if (!once) {
    once = true;
    init_entities(logger, registry, sps, ttable, gfx_state.gpu_state);
  }

  {
    update_nearbytargets(ldata, registry, ft);
    update_orbital_bodies(es, ldata, registry, ft);
    skybox.update(ft);
    move_riverwiggles(ldata, ft);

    {
      auto const player_eid = find_player(registry);
      auto const wp         = Player::world_position(player_eid, registry);
      auto const tp         = wp;
      // LOG_ERROR_SPRINTF("Player WP: %s TP: %s", glm::to_string(wp), glm::to_string(tp));
    }

    /*if (tilegrid_state.recompute) {
      // compute tilegrid
      LOG_INFO("Updating tilegrid\n");

      update_visible_tiles(ldata.tilegrid(), player, tilegrid_state.reveal);

      // We don't need to recompute the tilegrid, we just did.
      tilegrid_state.recompute = false;
    }
    */

    // river wiggles get updated every frame
    // update_visible_riverwiggles(ldata, player, tilegrid_state.reveal);

    update_visible_entities(lm, registry);
    update_torchflicker(ldata, registry, rng, ft);

    /*
    static bool once = false;
    if (!once) {
      once = true;
    registry.view<ShaderName, MeshRenderable, TreeLowpoly>().each(
        [&](auto eid, auto& sn, auto& mesh, auto& tree_component) {
          auto&      gfx_state = zs.gfx_state;
          auto&      sps       = gfx_state.sps;
          auto&      va        = sps.ref_sp(sn.value).va();
          auto const qa        = BufferFlags::from_va(va);
          auto const qo        = ObjQuery{mesh.name, qa};

          auto& entities_o = gfx_state.gpu_state.entities;
          assert(entities_o);
          auto& entity_map = *entities_o;
          auto& dinfo      = entity_map.lookup(logger, eid);

          //Tree::update_colors(logger, va, dinfo, tree_component);
        });
    }
    */
  }

  // TODO: Move out into state somewhere.

  auto const&      dim = es.dimensions;
  ScreenSize const screen_size{dim.w, dim.h};
  auto&            ti        = *ttable.find("water-diffuse");
  auto&            dudv      = *ttable.find("water-dudv");
  auto&            normal    = *ttable.find("water-normal");
  auto&            sp        = sps.ref_sp("water");
  auto const&      fog_color = ldata.fog.color;

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

  auto const make_water_renderer = [&]() {
    WaterFrameBuffers fbos{logger, screen_size, sp, ti, dudv, normal};
    return WaterRenderer{MOVE(fbos)};
  };

  // TODO: move these (they are static for convenience testing)
  static SkyboxRenderer skybox_renderer = make_skybox_renderer();
  static WaterRenderer  water_renderer  = make_water_renderer();

  DrawState ds;

  // Render the scene to the refraction and reflection FBOs
  bool const draw_water = es.draw_water;
  if (draw_water) {
    water_renderer.render_reflection(es, ds, lm, camera, skybox_renderer, rng, ft);
    water_renderer.render_refraction(es, ds, lm, camera, skybox_renderer, rng, ft);
  }

  // render scene
  render::clear_screen(ldata.fog.color);

  {
    auto const fmatrices = FrameMatrices::from_camera(camera);
    FrameState fstate{fmatrices, es, zs};

    RenderState rstate{fstate, ds};
    skybox_renderer.render(rstate, ds, ft);

    // The water must be drawn BEFORE rendering the scene the last time, otherwise it shows up ontop
    // of the ingame UI nearby target indicators.
    if (draw_water) {
      water_renderer.render_water(rstate, ds, lm, camera, ft);
    }

    // Render the scene with no culling (setting it zero disables culling mathematically)
    glm::vec4 const NOCULL_VECTOR{0, 0, 0, 0};
    render::render_scene(rstate, lm, rng, ft, NOCULL_VECTOR);
  }

  /*
  {
    glm::vec2 const pos{0.5f, -0.5f};
    glm::vec2 const scale{0.25f, 0.25f};

    render::draw_fbo_testwindow(fstate, pos, scale, waterfbos.refr());
  }
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
    ui_debug::draw(es, lm, engine.window, camera, ds, ft);
  }
}

} // namespace boomhs
