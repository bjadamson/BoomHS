#include <boomhs/level_assembler.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/constants.hpp>
#include <opengl/factory.hpp>
#include <opengl/lighting.hpp>

#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

ZoneState
assemble(LevelAssets &&assets, entt::DefaultRegistry &registry, LevelConfig const& config)
{
  auto const& objcache = assets.obj_cache;

  stlw::float_generator rng;
  auto leveldata = level_generator::make_leveldata(config, registry,
      MOVE(assets.tile_table), rng);

  // Load point lights
  auto light_view = registry.view<PointLight, Transform>();
  for (auto const entity : light_view) {
    auto &transform = light_view.get<Transform>(entity);
    transform.scale = glm::vec3{0.2f};
  }

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -Z_UNIT_VECTOR;
  auto constexpr UP = Y_UNIT_VECTOR;

  auto const player_eid = find_player(registry);
  EnttLookup player_lookup{player_eid, registry};
  WorldObject player{player_lookup, FORWARD, UP};
  Camera camera(player_lookup, FORWARD, UP);
  {
    SphericalCoordinates sc;
    sc.radius = 3.8f;
    sc.theta = glm::radians(-0.229f);
    sc.phi = glm::radians(38.2735f);
    camera.set_coordinates(MOVE(sc));
  }

  LevelState level_state{
    assets.background_color,
    assets.global_light,
    MOVE(assets.obj_cache),
    MOVE(leveldata),
    MOVE(camera),
    MOVE(player)
  };
  GfxState gfx{
    MOVE(assets.shader_programs),
    MOVE(assets.texture_table)
  };
  return ZoneState{
    MOVE(level_state),
    MOVE(gfx),
    registry};
}

void
bridge_staircases(ZoneState &a, ZoneState &b)
{
  auto &tilegrid_a = a.level_state.level_data.tilegrid();
  auto &tilegrid_b = b.level_state.level_data.tilegrid();

  auto const stairs_up_a = find_upstairs(a.registry, tilegrid_a);
  assert(!stairs_up_a.empty());

  auto const stairs_down_b = find_downstairs(b.registry, tilegrid_b);
  assert(!stairs_down_b.empty());

  auto &a_registry = a.registry;
  auto &b_registry = b.registry;

  auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;

  assert(stairs_up_a.size() == stairs_down_b.size());
  auto const num_stairs = stairs_up_a.size();
  FOR(i, num_stairs) {
    auto const a_updowneid = stairs_up_a[i];
    auto const b_downeid = stairs_down_b[i];

    assert(a_registry.has<StairInfo>(a_updowneid));
    StairInfo &si_a = a_registry.get<StairInfo>(a_updowneid);

    assert(b_registry.has<StairInfo>(b_downeid));
    StairInfo &si_b = b_registry.get<StairInfo>(b_downeid);

    // find a suitable neighbor tile for each stair
    auto const a_neighbors = find_immediate_neighbors(tilegrid_a, si_a.tile_position, TileType::FLOOR,
        behavior);
    assert(!a_neighbors.empty());

    auto const b_neighbors = find_immediate_neighbors(tilegrid_b, si_b.tile_position, TileType::FLOOR,
        behavior);
    assert(!b_neighbors.empty());

    // Set A's exit position to B's neighbor, and visa versa
    si_a.exit_position = b_neighbors.front();
    si_b.exit_position = a_neighbors.front();
  }
}

using copy_assets_pair_t = std::pair<EntityDrawHandles, TileDrawHandles>;
stlw::result<copy_assets_pair_t, std::string>
copy_assets_gpu(stlw::Logger &logger, ShaderPrograms &sps, TileSharedInfoTable const& ttable,
    entt::DefaultRegistry &registry, ObjCache const &obj_cache)
{
  EntityDrawinfos dinfos;
  /*
  registry.view<ShaderName, Color, CubeRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_colorcube_gpu(logger, shader_ref, color);
        dinfos.add(entity, MOVE(handle));
      });
      */
  registry.view<ShaderName, PointLight, CubeRenderable>().each(
      [&](auto entity, auto &sn, auto &pointlight, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_vertexonlycube_gpu(logger, shader_ref);
        dinfos.add(entity, MOVE(handle));
      });

  registry.view<ShaderName, Color, MeshRenderable>().each(
      [&](auto entity, auto &sn, auto &color, auto &mesh) {
        auto const &obj = obj_cache.get_obj(mesh.name);
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, std::nullopt);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, CubeRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &, auto &texture) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_texturecube_gpu(logger, shader_ref, texture.texture_info);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, SkyboxRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &, auto &texture) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_texturecube_gpu(logger, shader_ref, texture.texture_info);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &mesh, auto &texture) {
        auto const &obj = obj_cache.get_obj(mesh.name);
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, texture.texture_info);
        dinfos.add(entity, MOVE(handle));
      });

  registry.view<ShaderName, MeshRenderable>().each([&](auto entity, auto &sn, auto &mesh) {
    auto const &obj = obj_cache.get_obj(mesh.name);
    auto &shader_ref = sps.ref_sp(sn.value);
    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, std::nullopt);
    dinfos.add(entity, MOVE(handle));
  });

  std::vector<DrawInfo> tile_dinfos;
  tile_dinfos.reserve(static_cast<size_t>(TileType::MAX));
  for (auto const& it : ttable) {
    auto const& mesh_name = it.mesh_name;
    auto const& vshader_name = it.vshader_name;
    auto const &obj = obj_cache.get_obj(mesh_name);

    auto handle = OF::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(vshader_name), obj, std::nullopt);
    tile_dinfos[static_cast<size_t>(it.type)] = MOVE(handle);
  }

  EntityDrawHandles edh{MOVE(dinfos)};
  TileDrawHandles td{MOVE(tile_dinfos)};
  return std::make_pair(MOVE(edh), MOVE(td));
}

void
copy_to_gpu(stlw::Logger &logger, ZoneState &zs)
{
  auto &lstate = zs.level_state;
  auto const& ttable = lstate.level_data.tiletable();
  auto const& objcache = lstate.obj_cache;
  auto &gfx_state = zs.gfx_state;
  auto &sps = gfx_state.sps;
  auto &registry = zs.registry;

  auto copy_result = copy_assets_gpu(logger, sps, ttable, registry, objcache);
  assert(copy_result);
  auto handles = MOVE(*copy_result);
  auto edh = MOVE(handles.first);
  auto tdh = MOVE(handles.second);
  gfx_state.gpu_state.entities = MOVE(edh);
  gfx_state.gpu_state.tiles = MOVE(tdh);
}

} // ns anon

namespace boomhs
{

stlw::result<ZoneStates, std::string>
LevelAssembler::assemble_levels(stlw::Logger &logger, std::vector<entt::DefaultRegistry> &registries)
{
  auto const level_string = [&](int const floor_number)
  {
    return "area" + std::to_string(floor_number) + ".toml";
  };

  auto const stairs_perfloor = 8;
  int const width = 40, height = 40;
  TileGridConfig const tdconfig{width, height};

  // TODO: it is currently not thread safe to call load_level() from multiple threads.
  //
  // The logger isn't thread safe, need to ensure that the logger isn't using "during" level gen,
  // or somehow give it unique access during writing (read/write lock?).

  auto const FLOOR_COUNT = 2;
  std::vector<ZoneState> zstates;
  zstates.reserve(FLOOR_COUNT);
  FORI(i, FLOOR_COUNT) {
    auto &registry = registries[i];
    DO_TRY(auto level_assets, LevelLoader::load_level(logger, registry, level_string(i)));
    StairGenConfig const stairconfig{FLOOR_COUNT, i, stairs_perfloor};
    LevelConfig const level_config{stairconfig, tdconfig};

    ZoneState zs = assemble(MOVE(level_assets), registry, level_config);
    zstates.emplace_back(MOVE(zs));
  }

  assert(FLOOR_COUNT > 0);
  for(auto i = 1; i < FLOOR_COUNT; ++i) {
    bridge_staircases(zstates[i-1], zstates[i]);
  }

  // copy the first zonestate to GPU
  copy_to_gpu(logger, zstates.front());

  return ZoneStates{MOVE(zstates)};
}

} // ns boomhs
