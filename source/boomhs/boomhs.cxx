#include <boomhs/audio.hpp>
#include <boomhs/billboard.hpp>
#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/state.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water.hpp>


#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
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

bool
player_in_water(stlw::Logger& logger, EntityRegistry& registry)
{
  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);

  auto const eids = find_all_entities_with_component<WaterInfo, Transform, AABoundingBox>(registry);
  for (auto const eid : eids) {
    auto& water_bbox = registry.get<AABoundingBox>(eid);
    auto& w_tr       = registry.get<Transform>(eid);
    auto& p_tr       = player.transform();

    auto const& player_bbox = player.bounding_box();
    if (collision::bbox_intersects(logger, p_tr, player_bbox, w_tr, water_bbox)) {
      return true;
    }
  }
  return false;
}

void
update_playaudio(stlw::Logger& logger, LevelData& ldata, EntityRegistry& registry,
                 WaterAudioSystem& audio)
{
  if (player_in_water(logger, registry)) {
    audio.play_inwater_sound(logger);
  }
  else {
    audio.stop_inwater_sound(logger);
  }
}

void
update_npcpositions(stlw::Logger& logger, EntityRegistry& registry, TerrainGrid& terrain,
                    FrameTime const& ft)
{
  auto const update = [&](auto const eid) {
    auto& npcdata = registry.get<NPCData>(eid);
    auto& npc_hp = npcdata.health;
    if (NPC::is_dead(npc_hp)) {
      return;
    }

    auto& transform         = registry.get<Transform>(eid);
    auto const& bbox        = registry.get<AABoundingBox>(eid);

    auto& tr = transform.translation;
    float const height = terrain.get_height(logger, tr.x, tr.z);
    tr.y = height + (bbox.dimensions().y / 2.0f);
  };
  for (auto const eid : registry.view<NPCData, Transform, AABoundingBox>()) {
    update(eid);
  }
}

void
update_nearbytargets(NearbyTargets& nbt, EntityRegistry& registry, FrameTime const& ft)
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
    }
  };

  auto const eids = find_orbital_bodies(registry);
  if (es.update_orbital_bodies) {
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
      auto const player_eid = find_player(registry);
      auto const& player = registry.get<Player>(player_eid);
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
  auto& terrain_grid = ldata.terrain;

  for (auto const eid : registry.view<NPCData>()) {
    auto& isv = registry.get<IsVisible>(eid);
    isv.value        = true; //terrain.is_visible(registry);
  }
}

} // namespace

namespace boomhs
{

Result<stlw::Nothing, std::string>
copy_assets_gpu(stlw::Logger& logger, ShaderPrograms& sps,
                EntityRegistry& registry, ObjStore& obj_store, DrawHandleManager& draw_handles)
{
  auto const copy_cube = [&](auto const eid, auto& sn, auto& cr, auto&&...) {
    CubeMinMax const cmm{cr.min, cr.max};
    auto& va = sps.ref_sp(sn.value).va();

    auto const vertices = OF::cube_vertices(cmm.min, cmm.max);
    auto  handle = opengl::gpu::copy_cube_gpu(logger, vertices, va);
    draw_handles.add_entity(eid, MOVE(handle));
  };
  // copy CUBES to GPU
  registry.view<ShaderName, CubeRenderable>().each(
      [&](auto const eid, auto& sn, auto& cr, auto&&... args) {
      copy_cube(eid, sn, cr, FORWARD(args));
      });

  auto const copy_mesh = [&](auto const eid, auto& sn, auto& mesh, auto&&...) {
    auto&       va  = sps.ref_sp(sn.value).va();
    auto const  qa  = BufferFlags::from_va(va);
    auto const  qo  = ObjQuery{mesh.name, qa};
    auto const& obj = obj_store.get(logger, mesh.name);

    auto handle = opengl::gpu::copy_gpu(logger, va, obj);
    draw_handles.add_entity(eid, MOVE(handle));
  };

  // copy MESHES to GPU
  registry.view<ShaderName, MeshRenderable>().each(copy_mesh);

  // copy billboarded textures to GPU
  registry.view<ShaderName, BillboardRenderable, TextureRenderable>().each(
      [&](auto entity, auto& sn, auto&, auto& texture) {
        auto&      va = sps.ref_sp(sn.value).va();
        auto*      ti = texture.texture_info;
        assert(ti);

        auto const v  = OF::rectangle_vertices_default();
        auto const uv = OF::rectangle_uvs(ti->uv_max);
        auto const vertices = RectangleFactory::from_vertices_and_uvs(v, uv);
        auto handle = opengl::gpu::copy_rectangle_uvs(logger, va, vertices);
        draw_handles.add_entity(entity, MOVE(handle));
      });

  // Update the tree's to match their initial values.
  registry.view<ShaderName, MeshRenderable, TreeComponent>().each(
      [&](auto entity, auto& sn, auto& mesh, auto& tree) {
        auto& name = registry.get<MeshRenderable>(entity).name;

        auto&          va    = sps.ref_sp(sn.value).va();
        auto const     flags = BufferFlags::from_va(va);
        ObjQuery const query{name, flags};
        auto&          obj = obj_store.get(logger, name);

        auto& tc = registry.get<TreeComponent>(entity);
        tc.set_obj(&obj);

        auto& dinfo = draw_handles.lookup_entity(logger, entity);
        Tree::update_colors(logger, va, dinfo, tc);
      });

  return OK_NONE;
}

void
add_boundingboxes_to_entities(EngineState& es, ZoneState& zs)
{
  auto& logger = es.logger;
  auto& ldata     = zs.level_data;
  auto& registry  = zs.registry;

  auto& gfx_state    = zs.gfx_state;
  auto& sps          = gfx_state.sps;
  auto& draw_handles = gfx_state.draw_handles;

  auto& obj_store                 = ldata.obj_store;
  auto constexpr WIREFRAME_SHADER = "wireframe";
  auto& va                        = sps.ref_sp(WIREFRAME_SHADER).va();

  {
    CubeMinMax const cmm{glm::vec3{-1.0f}, glm::vec3{1.0f}};
    auto const cv = OF::cube_vertices(cmm.min, cmm.max);
    auto    dinfo = opengl::gpu::copy_cube_wireframe_gpu(logger, cv, va);
    draw_handles.set_bbox(MOVE(dinfo));
  }

  auto const          add_boundingbox = [&](auto const eid, auto const& min, auto const& max) {
    {
      auto& bbox = registry.assign<AABoundingBox>(eid);
      bbox.min   = min;
      bbox.max   = max;
    }
  };
  for (auto const eid : registry.view<MeshRenderable>()) {
    auto& name = registry.get<MeshRenderable>(eid).name;

    auto const     flags = BufferFlags::from_va(va);
    ObjQuery const query{name, flags};
    auto&          obj       = obj_store.get(logger, name);
    auto const     posbuffer = obj.positions();
    auto const&    min       = posbuffer.min();
    auto const&    max       = posbuffer.max();

    add_boundingbox(eid, min, max);
    registry.assign<Selectable>(eid);
  }
  for (auto const eid : registry.view<CubeRenderable>()) {
    auto const& cr = registry.get<CubeRenderable>(eid);

    add_boundingbox(eid, cr.min, cr.max);
    registry.assign<Selectable>(eid);
  }
  for (auto const eid : registry.view<OrbitalBody>()) {
    glm::vec3 constexpr min = glm::vec3{-1.0f};
    glm::vec3 constexpr max = glm::vec3{1.0f};
    add_boundingbox(eid, min, max);
    registry.assign<Selectable>(eid);
  }
  for (auto const eid : registry.view<WaterInfo>()) {
    {
      BufferFlags const flags{true, false, false, true};

      auto& wi = registry.get<WaterInfo>(eid);
      auto const dimensions = wi.dimensions;
      auto const num_vertexes = wi.num_vertexes;
      auto const data = WaterFactory::generate_water_data(logger, dimensions, num_vertexes);
      auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);

      auto& sp   = graphics_mode_to_water_shader(es.graphics_settings.mode, sps);
      auto dinfo = gpu::copy_gpu(logger, sp.va(), buffer);

      draw_handles.add_entity(eid, MOVE(dinfo));
      wi.eid = eid;
    }
  }

  for (auto const eid : registry.view<WaterInfo>()) {
    auto const min = glm::vec3{-0.5, -0.2, -0.5};
    auto const max = glm::vec3{0.5f, 0.2, 0.5};

    add_boundingbox(eid, min, max);
  }
}


ZoneState
assemble(LevelGeneratedData&& gendata, LevelAssets&& assets, EntityRegistry& registry)
{
  // Combine the generated data with the asset data, creating the LevelData instance.
  LevelData level_data{
                       MOVE(gendata.terrain),

                       MOVE(assets.fog),
                       assets.global_light,
                       MOVE(assets.material_table),
                       MOVE(assets.obj_store)};
  GfxState  gfx{MOVE(assets.shader_programs), MOVE(assets.texture_table)};
  return ZoneState{MOVE(level_data), MOVE(gfx), registry};
}

std::string
floornumber_to_levelfilename(int const floor_number)
{
  return "area" + std::to_string(floor_number) + ".toml";
}

Result<GameState, std::string>
init(Engine& engine, EngineState& es, Camera& camera, stlw::float_generator& rng)
{
  auto& logger = es.logger;

  int constexpr FLOOR_NUMBER = 0;

  std::vector<ZoneState> zstates;
  zstates.reserve(1);
  {
    auto& registry       = engine.registries[FLOOR_NUMBER];
    auto  level_name     = floornumber_to_levelfilename(FLOOR_NUMBER);
    auto  level_assets   = TRY_MOVEOUT(LevelLoader::load_level(logger, registry, level_name));
    auto& ttable         = level_assets.texture_table;
    auto& material_table = level_assets.material_table;
    auto& sps            = level_assets.shader_programs;

    char const* HEIGHTMAP_NAME = "Area0-HM";
    auto const  heightmap = TRY_MOVEOUT(heightmap::load_fromtable(logger, ttable, HEIGHTMAP_NAME));

    auto gendata = StartAreaGenerator::gen_level(logger, registry, rng, sps, ttable,
        material_table, heightmap);

    ZoneState zs = assemble(MOVE(gendata), MOVE(level_assets), registry);
    zstates.emplace_back(MOVE(zs));
  }
  {
    // copy the first zonestate to GPU
    assert(zstates.size() > 0);

    auto& zs = zstates.front();
    auto& ldata     = zs.level_data;
    auto& objstore  = ldata.obj_store;
    auto& gfx_state = zs.gfx_state;
    auto& sps       = gfx_state.sps;
    auto& registry  = zs.registry;

    auto& draw_handles = gfx_state.draw_handles;

    auto copy_result = TRY_MOVEOUT(copy_assets_gpu(logger, sps, registry, objstore, draw_handles));
    add_boundingboxes_to_entities(es, zs);
  }

  GameState state{es, LevelManager{MOVE(zstates)}};

  auto& lm       = state.level_manager;
  auto& zs       = lm.active();

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

  {
    auto& ingame = es.ui_state.ingame;
    auto& chat_history = ingame.chat_history;
    auto& chat_state = ingame.chat_state;

    chat_history.add_channel(0, "General", LOC::WHITE);
    chat_history.add_channel(1, "Group",   LOC::LIGHT_BLUE);
    chat_history.add_channel(2, "Guild",   LOC::LIGHT_GREEN);
    chat_history.add_channel(3, "Whisper", LOC::MEDIUM_PURPLE);
    chat_history.add_channel(4, "Area",    LOC::INDIAN_RED);

    auto const addmsg = [&](ChannelId const channel, auto &&msg) {
      Message m{channel, MOVE(msg)};
      chat_history.add_message(MOVE(m));
    };
    addmsg(0, "Welcome to the server 'Turnshroom Habitat'");
    addmsg(0, "Wizz: Hey");
    addmsg(0, "Thorny: Yo");
    addmsg(0, "Mufk: SUp man");
    addmsg(2, "Kazaghual: anyone w2b this axe I just found?");
    addmsg(2, "PizzaMan: Yo I'm here to deliver this pizza, I'll just leave it over here by the "
        "dragon ok?");
    addmsg(3, "Moo: grass plz");
    addmsg(4, "Aladin: STFU Jafar");
    addmsg(2, "Rocky: JKSLFJS");
    addmsg(1, "You took 31 damage.");
    addmsg(1, "You've given 25 damage.");
    addmsg(1, "You took 61 damage.");
    addmsg(1, "You've given 20 damage.");
    addmsg(2, R"(A gender chalks in the vintage coke. When will the murder pocket a wanted symptom?
              My
              attitude observes any nuisance into the laughing constant. Every candidate
              offers the railway under the beforehand molecule. The rescue buys his wrath
              underneath the above garble.
              The truth collars the bass into a lower heel. A squashed machinery kisses the
              abandon. Across its horse swims a sheep. Any umbrella damage rants over a sniff.

              How can a theorem chalk the frustrating fraud? Should the world wash an
              incomprehensible curriculum?)");

    addmsg(1, R"(The cap ducks inside the freedom. The mum hammers the apathy above our preserved
              ozone. Will the peanut nose a review species? His vocabulary beams near the virgin.

              The short supporter blames the hack fudge. The waffle exacts the bankrupt within an
              infantile attitude.)");
    addmsg(2, "A flesh hazards the sneaking tooth. An analyst steams before an instinct! The muscle "
              "expands within each brother! Why can't the indefinite garbage harden? The feasible "
              "cider moans in the forest.");
    addmsg(1, "Opposite the initiative scratches an inane plant. Why won't the late school "
              "experiment with a crown? The sneak papers a go dinner without a straw. How can an "
              "eating guy camp?"
          "Around the convinced verdict waffles a scratching shed. The "
              "inhabitant escapes before whatever outcry.");

    chat_state.reset_yscroll_position = true;
  }

  return OK_MOVE(state);
}

void
WaterRenderers::render(RenderState& rstate, DrawState& ds, LevelManager& lm, Camera& camera,
              FrameTime const& ft, bool const black_silhouette)
{
  if (black_silhouette) {
    silhouette.render_water(rstate, ds, lm, camera, ft);
  }
  else {
    auto const& water_buffer = rstate.fs.es.ui_state.debug.buffers.water;
    auto const  water_type = static_cast<GameGraphicsMode>(water_buffer.selected_water_graphicsmode);
    if (GameGraphicsMode::Basic == water_type) {
      basic.render_water(rstate, ds, lm, camera, ft);
    }
    else if (GameGraphicsMode::Medium == water_type) {
      medium.render_water(rstate, ds, lm, camera, ft);
    }
    else if (GameGraphicsMode::Advanced == water_type) {
      advanced.render_water(rstate, ds, lm, camera, ft);
    }
    else {
      std::abort();
    }
  }
}

void
StaticRenderers::render(LevelManager&lm, RenderState& rstate, stlw::float_generator& rng, DrawState& ds,
            FrameTime const& ft, bool const black_silhouette)
{
  // Render the scene with no culling (setting it zero disables culling mathematically)
  glm::vec4 const NOCULL_VECTOR{0, 0, 0, 0};

  auto& fs = rstate.fs;
  auto& es = fs.es;
  if (es.draw_terrain) {
    auto const draw_basic = [&](auto& terrain_renderer, auto& entity_renderer) {
      auto& zs = fs.zs;
      auto& ldata = zs.level_data;
      auto& registry = zs.registry;

      terrain_renderer.render(rstate, ldata.material_table, registry, ft, NOCULL_VECTOR);
    };
    if (black_silhouette) {
      draw_basic(silhouette_terrain, silhouette_entity);
    }
    else {
      draw_basic(default_terrain, default_entity);
    }
  }
  // DRAW ALL ENTITIES
  {
    if (es.draw_3d_entities) {
      if (black_silhouette) {
        silhouette_entity.render3d(rstate, rng, ft);
      }
      else {
        default_entity.render3d(rstate, rng, ft);
      }
    }
    if (es.draw_2d_billboard_entities) {
      if (black_silhouette) {
        silhouette_entity.render2d_billboard(rstate, rng, ft);
      }
      else {
        default_entity.render2d_billboard(rstate, rng, ft);
      }
    }
    if (es.draw_2d_ui_entities) {
      if (black_silhouette) {
        silhouette_entity.render2d_ui(rstate, rng, ft);
      }
      else {
        default_entity.render2d_ui(rstate, rng, ft);
      }
    }
  }
  if (black_silhouette) {
    // do nothing
  }
  else {
    debug.render_scene(rstate, lm, rng, ft);
  }
}

void
render_scene(RenderState& rstate, LevelManager& lm, DrawState& ds, Camera& camera,
             stlw::float_generator& rng, FrameTime const& ft, StaticRenderers& static_renderers)
{
  auto& es     = rstate.fs.es;
  auto& logger = es.logger;
  auto const& graphics_settings      = es.graphics_settings;
  bool const  graphics_mode_advanced = GameGraphicsMode::Advanced == graphics_settings.mode;

  auto const& water_buffer = es.ui_state.debug.buffers.water;
  bool const  draw_water = water_buffer.draw;
  bool const  draw_water_advanced    = draw_water && graphics_mode_advanced;

  auto& skybox_renderer = static_renderers.skybox;

  auto const draw_scene = [&](bool const black_silhouette) {
    auto& water_renderer = static_renderers.water;
    auto const draw_advanced = [&](auto& terrain_renderer, auto& entity_renderer) {
      water_renderer.advanced.render_reflection(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
      water_renderer.advanced.render_refraction(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, rng, ft);
    };
    if (draw_water && draw_water_advanced && !black_silhouette) {
      // Render the scene to the refraction and reflection FBOs
      draw_advanced(static_renderers.default_terrain, static_renderers.default_entity);
    }

    // render scene
    if (es.draw_skybox) {
      auto const& zs    = lm.active();
      auto const& ldata = zs.level_data;
      auto const clear_color = black_silhouette ? LOC::BLACK : ldata.fog.color;
      render::clear_screen(clear_color);

      if (!black_silhouette) {
        skybox_renderer.render(rstate, ds, ft);
      }
    }

    // The water must be drawn BEFORE rendering the scene the last time, otherwise it shows up
    // ontop of the ingame UI nearby target indicators.
    if (draw_water) {
      water_renderer.render(rstate, ds, lm, camera, ft, black_silhouette);
    }

    static_renderers.render(lm, rstate, rng, ds, ft, black_silhouette);
  };

  auto const draw_scene_normal_render = [&]() { draw_scene(false); };

  auto const render_scene_with_sunshafts = [&]() {
    // draw scene with black silhouttes into the sunshaft FBO.
    auto& sunshaft_renderer = static_renderers.sunshaft;
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
}

StaticRenderers
make_static_renderers(EngineState& es, LevelManager& lm)
{
  auto const make_basic_water_renderer = [](stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable) {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = graphics_mode_to_water_shader(GameGraphicsMode::Basic, sps);
    return BasicWaterRenderer{logger, diff, normal, sp};
  };

  auto const make_medium_water_renderer = [](stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable) {
    auto& diff   = *ttable.find("water-diffuse");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = graphics_mode_to_water_shader(GameGraphicsMode::Medium, sps);
    return MediumWaterRenderer{logger, diff, normal, sp};
  };

  auto const       make_advanced_water_renderer = [](EngineState& es, ZoneState& zs) {
    auto& logger   = es.logger;
    auto const&      dim = es.dimensions;
    ScreenSize const screen_size{dim.right, dim.bottom};

    auto& gfx_state = zs.gfx_state;
    auto& ttable    = gfx_state.texture_table;
    auto& sps       = gfx_state.sps;
    auto& ti     = *ttable.find("water-diffuse");
    auto& dudv   = *ttable.find("water-dudv");
    auto& normal = *ttable.find("water-normal");
    auto& sp     = graphics_mode_to_water_shader(GameGraphicsMode::Advanced, sps);
    return AdvancedWaterRenderer{logger, screen_size, sp, ti, dudv, normal};
  };

  auto const make_black_water_renderer = [](EngineState& es, ZoneState& zs) {
    auto& logger   = es.logger;
    auto& gfx_state = zs.gfx_state;
    auto& sps       = gfx_state.sps;
    auto& sp = sps.ref_sp("silhoutte_black");
    return BlackWaterRenderer{logger, sp};
  };

  auto const make_black_terrain_renderer = [](ShaderPrograms& sps) {
    auto& sp = sps.ref_sp("silhoutte_black");
    return BlackTerrainRenderer{sp};
  };

  auto const make_sunshaft_renderer = [](EngineState& es, ZoneState& zs) {
    auto& logger   = es.logger;
    auto& gfx_state = zs.gfx_state;
    auto& sps       = gfx_state.sps;
    auto& sunshaft_sp = sps.ref_sp("sunshaft");
    auto const&      dim = es.dimensions;
    ScreenSize const screen_size{dim.right, dim.bottom};
    return SunshaftRenderer{logger, screen_size, sunshaft_sp};
  };

  // TODO: Move out into state somewhere.
  auto const make_skybox_renderer = [](stlw::Logger& logger, ShaderPrograms& sps, TextureTable& ttable) {
    auto&              skybox_sp = sps.ref_sp("skybox");
    glm::vec3 const    vmin{-0.5f};
    glm::vec3 const    vmax{0.5f};
    CubeMinMax const cmm{vmin, vmax};

    auto const vertices = OF::cube_vertices(cmm.min, cmm.max);
    DrawInfo           dinfo    = opengl::gpu::copy_cube_gpu(logger, vertices, skybox_sp.va());
    auto&              day_ti   = *ttable.find("building_skybox");
    auto&              night_ti = *ttable.find("night_skybox");
    return SkyboxRenderer{logger, MOVE(dinfo), day_ti, night_ti, skybox_sp};
  };

  auto& logger   = es.logger;

  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;
  return StaticRenderers{
    DefaultTerrainRenderer{},
    make_black_terrain_renderer(sps),
    EntityRenderer{},
    BlackEntityRenderer{},
    make_skybox_renderer(logger, sps, ttable),
    make_sunshaft_renderer(es, zs),
    DebugRenderer{},

    WaterRenderers{
      make_basic_water_renderer(logger, sps, ttable),
      make_medium_water_renderer(logger, sps, ttable),
      make_advanced_water_renderer(es, zs),
      make_black_water_renderer(es, zs)
    }
  };
}

void
ingame_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
            WaterAudioSystem& water_audio, StaticRenderers& static_renderers, DrawState& ds,
            FrameTime const& ft)
{
  auto& es = state.engine_state;
  es.time.update(ft.since_start_seconds());

  auto& skybox_renderer = static_renderers.skybox;

  auto& logger   = es.logger;
  auto& lm       = state.level_manager;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  auto& ldata  = zs.level_data;
  auto& skybox = ldata.skybox;

  auto& gfx_state = zs.gfx_state;
  auto& ttable    = gfx_state.texture_table;

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);

  auto& nbt = ldata.nearby_targets;
  auto const cstate = CameraFrameState::from_camera(camera);
  FrameState fstate{cstate, es, zs};
  {
    // Update the world
    update_playaudio(logger, ldata, registry, water_audio);

    auto const view_matrix = fstate.view_matrix();
    auto const proj_matrix = fstate.projection_matrix();
    update_orbital_bodies(es, ldata, view_matrix, proj_matrix, registry, ft);
    skybox.update(ft);

    update_visible_entities(lm, registry);
    update_torchflicker(ldata, registry, rng, ft);

    // Update these as a chunk, so they stay in the correct order.
    {
      auto& terrain = ldata.terrain;
      update_npcpositions(logger, registry, terrain, ft);
      update_nearbytargets(nbt, registry, ft);
      player.update(logger, registry, terrain, ttable, nbt);
    }
  }

  {
    RenderState rstate{fstate, ds};
    render_scene(rstate, lm, ds, camera, rng, ft, static_renderers);
  }

  auto& ui_state = es.ui_state;
  if (ui_state.draw_ingame_ui) {
    ui_ingame::draw(es, lm, camera, ds);
  }
}

void
game_loop(Engine& engine, GameState& state, StaticRenderers& static_renderers,
          stlw::float_generator& rng, Camera& camera, FrameTime const& ft)
{
  auto& es        = state.engine_state;
  auto& logger      = es.logger;
  auto& lm        = state.level_manager;

  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  auto& io = es.imgui;

  auto& registry = zs.registry;
  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);

  static bool set_camera_once = false;
  if (!set_camera_once) {
    camera.set_target(player.world_object);
    set_camera_once = true;
  }

  static auto audio_r     = WaterAudioSystem::create();
  static auto water_audio = audio_r.expect_moveout("WAS");

  DrawState ds;
  if (es.main_menu.show) {
    // Enable keyboard shortcuts
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // clear the screen before rending the main menu
    render::clear_screen(LOC::BLACK);

    auto const& dimensions = engine.dimensions();
    auto const size_v      = ImVec2(dimensions.right, dimensions.bottom);
    main_menu::draw(es, size_v, water_audio);
  }
  else {
    // Disable keyboard shortcuts
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    IO::process(state, engine.controllers, camera, ft);

    ingame_loop(engine, state, rng, camera, water_audio, static_renderers, ds, ft);
  }

  auto& ui_state = es.ui_state;
  if (ui_state.draw_debug_ui) {
    auto& lm = state.level_manager;
    auto& skybox_renderer = static_renderers.skybox;
    ui_debug::draw(es, lm, skybox_renderer, water_audio, engine.window, camera, ds, ft);
  }
}

} // namespace boomhs
