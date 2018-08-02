#include <boomhs/audio.hpp>
#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/rexpaint.hpp>
#include <boomhs/state.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_ingame.hpp>
#include <boomhs/water.hpp>

#include <opengl/entity_renderer.hpp>
#include <opengl/frame.hpp>
#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>
#include <opengl/scene_renderer.hpp>
#include <opengl/skybox_renderer.hpp>
#include <opengl/sun_renderer.hpp>
#include <opengl/terrain_renderer.hpp>
#include <opengl/texture.hpp>
#include <opengl/water_renderer.hpp>

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
  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);

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
  auto& wi    = WaterFactory::make_default(logger, sps, ttable, registry);
  wi.position = pos;

  size_t constexpr num_vertexes = 4;
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

Result<GameState, std::string>
init(Engine& engine, EngineState& es, Camera& camera)
{
  auto& logger = es.logger;

  auto assembled = LevelAssembler::assemble_levels(logger, engine.registries);
  ZoneStates zss = TRY_MOVEOUT(MOVE(assembled));

  GameState state{es, LevelManager{MOVE(zss)}};

  auto& lm       = state.level_manager;
  auto& zs       = lm.active();
  auto& registry = zs.registry;

  {
    auto const player_eid = find_player(registry);
    auto&      transform  = registry.get<Transform>(player_eid);
    camera.set_target(transform);
  }

  for (auto& zs : state.level_manager) {
    auto& sps = zs.gfx_state.sps;

    auto& water_sp = draw_water_options_to_shader(GameGraphicsMode::Basic, sps);
    place_water(logger, zs, water_sp, glm::vec2{0.0f, 0.0f});
    //place_water(logger, zs, water_sp, glm::vec2{20.0f, 20.0f});
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

  return Ok(MOVE(state));
}

void
ingame_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
            WaterAudioSystem& water_audio, SkyboxRenderer& skybox_renderer, DrawState& ds,
            FrameTime const& ft)
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
  ScreenSize const screen_size{dim.right, dim.bottom};
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

  auto const make_sunshaft_renderer = [&]() {
    auto& sunshaft_sp = sps.ref_sp("sunshaft");
    return SunshaftRenderer{logger, screen_size, sunshaft_sp};
  };

  // TODO: move these (they are static for convenience testing)
  static auto           basic_water_renderer    = make_basic_water_renderer();
  static auto           medium_water_renderer   = make_medium_water_renderer();
  static auto           advanced_water_renderer = make_advanced_water_renderer();
  static auto           black_water_renderer    = make_black_water_renderer();

  static auto default_terrain_renderer  = DefaultTerrainRenderer{};
  static auto black_terrain_renderer  = make_black_terrain_renderer();
  static auto default_entity_renderer = EntityRenderer{};
  static auto black_entity_renderer   = BlackEntityRenderer{};
  static auto default_scene_renderer  = DefaultSceneRenderer{};
  static auto black_scene_renderer    = BlackSceneRenderer{};

  static auto sunshaft_renderer = make_sunshaft_renderer();

  auto const fmatrices = FrameMatrices::from_camera(camera);
  FrameState fstate{fmatrices, es, zs};

  RenderState rstate{fstate, ds};

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);
  auto& nbt = ldata.nearby_targets;

  {
    // Update the world
    update_playaudio(logger, ldata, registry, water_audio);

    update_orbital_bodies(es, ldata, fstate.view_matrix(), fstate.projection_matrix(), registry,
                          ft);
    skybox.update(ft);

    update_visible_entities(lm, registry);
    update_torchflicker(ldata, registry, rng, ft);

    // Update these as a chunk, so they stay in the correct order.
    {
      auto& terrain = ldata.terrain;
      update_npcpositions(logger, registry, terrain, ft);
      update_nearbytargets(nbt, registry, ft);
      player.update(logger, registry, terrain, nbt);
    }
  }

  auto const& water_buffer = es.ui_state.debug.buffers.water;
  auto const  water_type = static_cast<GameGraphicsMode>(water_buffer.selected_water_graphicsmode);
  bool const  draw_water = water_buffer.draw;

  auto const& graphics_settings      = es.graphics_settings;
  bool const  graphics_mode_advanced = GameGraphicsMode::Advanced == graphics_settings.mode;
  bool const  draw_water_advanced    = draw_water && graphics_mode_advanced;

  auto const draw_scene = [&](bool const black_silhoutte) {
    auto const draw_advanced = [&](auto& terrain_renderer, auto& entity_renderer, auto& scene_renderer) {
      advanced_water_renderer.render_reflection(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, scene_renderer, rng, ft);
      advanced_water_renderer.render_refraction(es, ds, lm, camera, entity_renderer,
                                                skybox_renderer, terrain_renderer, scene_renderer, rng, ft);
    };
    if (draw_water && draw_water_advanced && !black_silhoutte) {
      // Render the scene to the refraction and reflection FBOs
      draw_advanced(default_terrain_renderer, default_entity_renderer, default_scene_renderer);
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
        terrain_renderer.render(rstate, ldata.material_table, registry, ft, NOCULL_VECTOR);
      };
      if (black_silhoutte) {
        draw_basic(black_terrain_renderer, black_entity_renderer);
      }
      else {
        draw_basic(default_terrain_renderer, default_entity_renderer);
      }
    }
    // DRAW ALL ENTITIES
    {
      if (es.draw_3d_entities) {
        if (black_silhoutte) {
          black_entity_renderer.render3d(rstate, rng, ft);
        }
        else {
          default_entity_renderer.render3d(rstate, rng, ft);
        }
      }
      if (es.draw_2d_billboard_entities) {
        if (black_silhoutte) {
          black_entity_renderer.render2d_billboard(rstate, rng, ft);
        }
        else {
          default_entity_renderer.render2d_billboard(rstate, rng, ft);
        }
      }
      if (es.draw_2d_ui_entities) {
        if (black_silhoutte) {
          black_entity_renderer.render2d_ui(rstate, rng, ft);
        }
        else {
          default_entity_renderer.render2d_ui(rstate, rng, ft);
        }
      }
    }
    if (black_silhoutte) {
      black_scene_renderer.render_scene(rstate, lm, rng, ft);
    }
    else {
      default_scene_renderer.render_scene(rstate, lm, rng, ft);
    }
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


  auto const dimensions = engine.dimensions();
  auto const draw_icon_on_screen = [&](auto const& pos, auto const& size, char const* tex_name)
  {
    auto& ti = *ttable.find(tex_name);
    auto& sp  = sps.ref_sp("2dtexture_ss");

    // Create a renderstate using an orthographic projection.
    auto const fmatrices = FrameMatrices::from_camera_with_mode(camera, CameraMode::Ortho);
    FrameState fstate{fmatrices, es, zs};
    RenderState rstate_2dtexture{fstate, ds};

    class BlinkTimer
    {
      bool is_blinking_ = false;
      Timer timer_;
    public:
      void update() { timer_.update(); }
      bool expired() const { return timer_.expired(); }
      bool is_blinking() const { return is_blinking_; }
      void toggle() { is_blinking_ ^= true; }
      void set_ms(double const t) { timer_.set_ms(t); }
    };

    static BlinkTimer blink_timer;
    blink_timer.update();

    bool const is_expired       = blink_timer.expired();
    bool const player_attacking = player.is_attacking;

    auto const reset_attack_timer = [&]() {
      auto const BLINK_TIME_IN_MS = TimeConversions::seconds_to_millis(1);
      blink_timer.set_ms(BLINK_TIME_IN_MS);
    };

    if (player_attacking) {
      if (is_expired) {
        blink_timer.toggle();
        reset_attack_timer();
      }
    }
    else if (is_expired) {
      reset_attack_timer();
    }

    bool const using_blink_shader = player_attacking && blink_timer.is_blinking();
    auto& spp = using_blink_shader
        ? sps.ref_sp("2d_ui_blinkcolor_icon")
        : sp;
    if (using_blink_shader) {

      auto const& player_transform = player.transform();
      auto const player_pos = player_transform.translation;

      // We can only be considering the blink shader if we are attacking an entity, and if we are
      // attacking an entity that means their should be a selected target.
      auto const selected_nbt = nbt.selected();
      assert(selected_nbt);
      auto const target_eid = *selected_nbt;
      auto const& target = registry.get<Transform>(target_eid);
      auto const& target_pos = target.translation;

      auto const distance = glm::distance(player_pos, target_pos);
      bool const close_enough = distance < 2;
      //LOG_ERROR_SPRINTF("close: %i, distance: %f, ppos: %s tpos: %s",
          //close_enough,
          //distance,
          //glm::to_string(player_pos),
          //glm::to_string(target_pos)
          //);
      auto const blink_color = NPC::within_attack_range(player_pos, target_pos)
        ? LOC::RED
        : LOC::BLUE;
      spp.while_bound(logger, [&]() {
          spp.set_uniform_color(logger, "u_blinkcolor", blink_color);
      });
    }
    render::draw_fbo_testwindow(rstate_2dtexture, pos, size, spp, ti);
  };

  auto const draw_slot_icon = [&](auto const slot_pos, char const* icon_name) {
    glm::vec2 const size{32.0f};

    auto constexpr SPACE_BETWEEN = 10;
    float const left   = dimensions.left + 39 + 200 + SPACE_BETWEEN
      + (slot_pos * (size.x + SPACE_BETWEEN));


    auto constexpr SPACE_BENEATH = 10;
    float const bottom = dimensions.bottom - SPACE_BENEATH - size.y;
    glm::vec2 const pos{left, bottom};
    draw_icon_on_screen(pos, size, icon_name);
  };

  draw_slot_icon(0, "fist");
  draw_slot_icon(1, "sword");
  draw_slot_icon(2, "ak47");
  draw_slot_icon(3, "first-aid");

  {
    glm::vec2 const size{128.0f};
    glm::vec2 const pos{dimensions.right - size.x, dimensions.bottom - size.y};
    draw_icon_on_screen(pos, size, "sword");
  }

  auto& ui_state = es.ui_state;
  if (ui_state.draw_ingame_ui) {
    ui_ingame::draw(es, lm);
  }
}

void
game_loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
          FrameTime const& ft)
{
  auto& es        = state.engine_state;
  auto& logger      = es.logger;
  auto& lm        = state.level_manager;

  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& sps       = gfx_state.sps;
  auto& ttable    = gfx_state.texture_table;

  auto& io = es.imgui;

  static auto audio_r     = WaterAudioSystem::create();
  static auto water_audio = audio_r.expect_moveout("WAS");

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

  static SkyboxRenderer skybox_renderer         = make_skybox_renderer();

  DrawState ds;
  if (es.main_menu.show) {
    // Enable keyboard shortcuts
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // clear the screen before rending the main menu1
    render::clear_screen(LOC::BLACK);

    auto const& dimensions = engine.dimensions();
    auto const size_v      = ImVec2(dimensions.right, dimensions.bottom);
    main_menu::draw(es, size_v, water_audio);
  }
  else {
    // Disable keyboard shortcuts
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    IO::process(state, engine.controllers, camera, ft);

    ingame_loop(engine, state, rng, camera, water_audio, skybox_renderer, ds, ft);
  }

  auto& ui_state = es.ui_state;
  if (ui_state.draw_debug_ui) {
    auto& lm = state.level_manager;
    ui_debug::draw(es, lm, skybox_renderer, water_audio, engine.window, camera, ds, ft);
  }
}

} // namespace boomhs
