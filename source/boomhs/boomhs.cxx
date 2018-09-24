#include <boomhs/audio.hpp>
#include <boomhs/billboard.hpp>
#include <boomhs/boomhs.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/controller.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/npc.hpp>

#include <boomhs/ortho_renderer.hpp>
#include <boomhs/perspective_renderer.hpp>
#include <boomhs/scene_renderer.hpp>

#include <boomhs/player.hpp>
#include <boomhs/rexpaint.hpp>

#include <boomhs/random.hpp>
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

#include <common/log.hpp>
#include <common/result.hpp>

#include <extlibs/fastnoise.hpp>
#include <extlibs/imgui.hpp>
#include <extlibs/sdl.hpp>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;
using namespace gl_sdl;

namespace
{

bool
player_in_water(common::Logger& logger, EntityRegistry& registry)
{
  auto const& player = find_player(registry);
  auto const& player_bbox = player.bounding_box().cube;
  auto const& p_tr        = player.transform();

  auto const eids = find_all_entities_with_component<WaterInfo, Transform, AABoundingBox>(registry);
  for (auto const eid : eids) {
    auto const& water_bbox = registry.get<AABoundingBox>(eid).cube;
    auto const& water_info = registry.get<WaterInfo>(eid);
    auto  w_tr       = registry.get<Transform>(eid);
    w_tr.scale.x = water_info.dimensions.x;
    w_tr.scale.z = water_info.dimensions.y;

    if (collision::cube_intersects(logger, p_tr, player_bbox, w_tr, water_bbox)) {
      return true;
    }
  }
  return false;
}

void
update_mousestates(EngineState& es)
{
  auto& mss = es.device_states.mouse;
  mss.previous = mss.current;
}

void
update_playaudio(common::Logger& logger, LevelData& ldata, EntityRegistry& registry,
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
set_heights_ontop_terrain(common::Logger& logger, TerrainGrid& terrain,
                               EntityRegistry& registry, EntityID const eid)
{
  auto& transform    = registry.get<Transform>(eid);
  auto const& bbox   = registry.get<AABoundingBox>(eid).cube;
  auto &tr           = transform.translation;
  float const height = terrain.get_height(logger, tr.x, tr.z);

  // update original transform
  tr.y = bbox.half_widths().y + height;
}

void
update_npcpositions(common::Logger& logger, EntityRegistry& registry, TerrainGrid& terrain,
                    FrameTime const& ft)
{
  auto const update = [&](auto const eid) {
    auto& npcdata = registry.get<NPCData>(eid);
    auto& npc_hp = npcdata.health;
    if (NPC::is_dead(npc_hp)) {
      return;
    }
    set_heights_ontop_terrain(logger, terrain, registry, eid);
  };
  for (auto const eid : registry.view<NPCData, Transform, AABoundingBox>()) {
    update(eid);
  }
}

void
update_nearbytargets(NearbyTargets& nbt, EntityRegistry& registry, FrameTime const& ft)
{
  auto const& player = find_player(registry);

  auto const enemies = find_enemies(registry);
  using pair_t       = std::pair<float, EntityID>;
  std::vector<pair_t> pairs;
  for (auto const eid : enemies) {
    if (registry.get<IsRenderable>(eid).hidden) {
      continue;
    }
    auto const& etransform = registry.get<Transform>(eid);
    float const distance   = glm::distance(player.transform().translation, etransform.translation);
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

    pos.x = orbital.radius.x * cos_time;
    pos.y = orbital.radius.y * sin_time;
    pos.z = orbital.radius.z * sin_time;

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
update_torchflicker(LevelData const& ldata, EntityRegistry& registry, RNG& rng,
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
      auto const& player = find_player(registry);
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
    auto& isr = registry.get<IsRenderable>(eid);
    isr.hidden        = false; //terrain.is_visible(registry);
  }
}

void
update_everything(EngineState& es, LevelManager& lm, RNG& rng, FrameState const& fstate, Camera& camera,
                  StaticRenderers& static_renderers, WaterAudioSystem& water_audio,
                  SDLWindow& window, FrameTime const& ft)
{
  auto& skybox_renderer = static_renderers.skybox;

  auto& logger   = es.logger;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  auto& ldata  = zs.level_data;
  auto& skybox = ldata.skybox;

  auto& gfx_state = zs.gfx_state;
  auto& ttable    = gfx_state.texture_table;

  auto& player = find_player(registry);
  auto& nbt = ldata.nearby_targets;

  // THIS GOES FIRST ALWAYS.
  es.time.update(ft.since_start_seconds());

  // Update the world
  update_playaudio(logger, ldata, registry, water_audio);

  auto const view_matrix = fstate.view_matrix();
  auto const proj_matrix = fstate.projection_matrix();
  update_orbital_bodies(es, ldata, view_matrix, proj_matrix, registry, ft);
  skybox.update(ft);

  update_visible_entities(lm, registry);
  update_torchflicker(ldata, registry, rng, ft);

  auto const is_target_selected_and_alive = [](EntityRegistry& registry, NearbyTargets const& nbt) {
    auto const target = nbt.selected();
    if (!target) {
      return false; // target not selected
    }
    auto const target_eid = *target;
    auto& npcdata = registry.get<NPCData>(target_eid);
    auto& target_hp = npcdata.health;
    return !NPC::is_dead(target_hp);
  };

  // Update these as a chunk, so they stay in the correct order.
  auto& terrain = ldata.terrain;
  update_npcpositions(logger, registry, terrain, ft);
  update_nearbytargets(nbt, registry, ft);

  //LOG_ERROR_SPRINTF("ortho cam pos: %s, player pos: %s",
      //glm::to_string(camera.ortho.position),
      //glm::to_string(player.transform().translation));

  //auto& terrain = ldata.terrain;
  //for (auto const eid : registry.view<Transform, MeshRenderable>()) {
    //set_heights_ontop_terrain(logger, terrain, registry, eid);
  //}

  bool const previously_alive = is_target_selected_and_alive(registry, nbt);
  player.update(es, zs, ft);

  if (previously_alive) {
    auto const target = nbt.selected();
    if (target) {
      auto const target_eid = *target;
      auto& npcdata = registry.get<NPCData>(target_eid);
      auto& target_hp = npcdata.health;
      bool const target_dead_after_attack = NPC::is_dead(target_hp);
      bool const dead_from_attack = previously_alive && target_dead_after_attack;

      auto const add_worlditem_at_targets_location = [&](EntityID const item_eid) {
        auto& item_tr = registry.get<Transform>(item_eid);

        auto const& target_pos = registry.get<Transform>(target_eid).translation;
        item_tr.translation = target_pos;
        item_tr.rotate_degrees(90, math::EulerAxis::X);
        auto const& item_name = registry.get<Name>(item_eid).value;
        LOG_ERROR_SPRINTF("ADDING item %s AT xyz: %s", item_name, glm::to_string(item_tr.translation));

        auto& ldata     = zs.level_data;
        auto& obj_store = ldata.obj_store;
        auto& sps       = gfx_state.sps;

        // copy the item to th GPU
        auto& dhm = gfx_state.draw_handles;
        dhm.add_mesh(logger, sps, obj_store, item_eid, registry);
      };
      if (dead_from_attack) {
        auto  const book_eid = ItemFactory::create_book(registry, ttable);
        add_worlditem_at_targets_location(book_eid);

        auto  const spear_eid = ItemFactory::create_spear(registry, ttable);
        add_worlditem_at_targets_location(spear_eid);
      }
    }
  }

  update_mousestates(es);
}


} // namespace

namespace boomhs
{

Result<common::Nothing, std::string>
copy_assets_gpu(common::Logger& logger, ShaderPrograms& sps,
                EntityRegistry& registry, ObjStore& obj_store, DrawHandleManager& dhm)
{
  // copy CUBES to GPU
  registry.view<ShaderName, CubeRenderable>().each(
      [&](auto const eid, auto&&...) {
      dhm.add_cube(logger, sps, eid, registry);
      });

  // copy MESHES to GPU
  registry.view<ShaderName, MeshRenderable>().each(
      [&](auto const eid, auto&&...) {
      dhm.add_mesh(logger, sps, obj_store, eid, registry);
      });

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
        dhm.add_entity(entity, MOVE(handle));
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
        auto& dinfo = dhm.lookup_entity(logger, entity);
        Tree::update_colors(logger, va, dinfo, tc);
      });

  return OK_NONE;
}

void
add_orbitalbodies_and_water(EngineState& es, ZoneState& zs)
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

  for (auto const eid : registry.view<OrbitalBody>()) {
    auto constexpr MIN = glm::vec3{-1.0f};
    auto constexpr MAX = glm::vec3{-1.0f};
    AABoundingBox::add_to_entity(logger, sps, eid, registry, MIN, MAX);
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
init(Engine& engine, EngineState& es, Camera& camera, RNG& rng)
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
        material_table, heightmap, camera.world_orientation_ref());

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
    add_orbitalbodies_and_water(es, zs);

    auto& terrain = ldata.terrain;
    for (auto const eid : registry.view<Transform, AABoundingBox, MeshRenderable>()) {
      //set_heights_ontop_terrain(logger, terrain, registry, eid);
    }
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
draw_everything(FrameState& fs, LevelManager& lm, RNG& rng, Camera& camera,
            WaterAudioSystem& water_audio, StaticRenderers& static_renderers, DrawState& ds,
            FrameTime const& ft)
{
  auto& es        = fs.es;
  auto& zs        = lm.active();
  auto const& dim = es.dimensions;
  {
    RenderState rstate{fs, ds};

    auto const mode = camera.mode();
    if (CameraMode::FPS == mode || CameraMode::ThirdPerson == mode) {
      render::set_viewport(es.dimensions);
      PerspectiveRenderer::draw_scene(rstate, lm, ds, camera, rng, static_renderers, ft);

      float const right  = dim.right();
      float const bottom = dim.bottom();

      auto& io = es.imgui;
      io.DisplaySize = ImVec2{right, bottom};
      auto& ui_state = es.ui_state;
      if (ui_state.draw_ingame_ui) {
        ui_ingame::draw(fs, camera, ds);
      }
      if (ui_state.draw_debug_ui) {

        auto static constexpr WINDOW_FLAGS = (0
          | ImGuiWindowFlags_AlwaysAutoResize
        );
        ui_debug::draw("Perspective", WINDOW_FLAGS, es, lm, camera, ft);
      }
    }
    else if (CameraMode::Ortho == mode) {
      OrthoRenderer::draw_scene(rstate, lm, ds, camera, rng, static_renderers, ft);
    }
    else {
      std::abort();
    }
  }
}

void
game_loop(Engine& engine, GameState& state, StaticRenderers& static_renderers,
          RNG& rng, Camera& camera, FrameTime const& ft)
{
  auto& es        = state.engine_state;
  auto& logger      = es.logger;
  auto& lm        = state.level_manager;

  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  auto& registry = zs.registry;
  auto& player = find_player(registry);

  static bool set_camera_once = false;
  if (!set_camera_once) {
    camera.set_target(player.head_world_object());
    set_camera_once = true;
  }

  static auto water_audio = WaterAudioSystem::create()
    .expect_moveout("Water Audio System");
  water_audio.set_volume(es.ui_state.debug.buffers.audio.ambient);

  DrawState ds{es.wireframe_override};

  auto& io = es.imgui;
  auto const& dimensions = engine.dimensions();
  if (es.main_menu.show) {
    SDL_SetRelativeMouseMode(SDL_FALSE);

    // Enable keyboard shortcuts
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // clear the screen before rending the main menu
    render::clear_screen(LOC::BLACK);
    render::set_viewport(dimensions);

    auto& skybox_renderer  = static_renderers.skybox;
    main_menu::draw(es, engine.window, camera, skybox_renderer, ds, lm, dimensions, water_audio);
  }
  else {
    {
      bool const fps_mode = camera.mode() == CameraMode::FPS;
      auto const relative_mode = fps_mode ? SDL_TRUE : SDL_FALSE;
      assert(0 == SDL_SetRelativeMouseMode(relative_mode));
    }

    // Disable keyboard shortcuts
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    IO_SDL::read_devices(SDLReadDevicesArgs{state, engine.controllers, camera, ft});
    SDL_SetCursor(es.device_states.cursors.active());

    auto fs         = FrameState::from_camera(es, zs, camera, camera.view_settings_ref(), es.frustum);
    update_everything(es, lm, rng, fs, camera, static_renderers, water_audio, engine.window, ft);
    draw_everything(fs, lm, rng, camera, water_audio, static_renderers, ds, ft);
  }
}

} // namespace boomhs
