#include <boomhs/level_assembler.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/components.hpp>
#include <boomhs/dungeon_generator.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/start_area_generator.hpp>

#include <opengl/constants.hpp>
#include <opengl/gpu.hpp>
#include <boomhs/obj.hpp>
#include <opengl/lighting.hpp>

#include <stlw/os.hpp>
#include <stlw/random.hpp>
#include <sstream>

using namespace boomhs;
using namespace opengl;

namespace
{

ZoneState
assemble(LevelGeneredData &&gendata, LevelAssets &&assets, EntityRegistry &registry)
{
  // Load point lights
  auto light_view = registry.view<PointLight, Transform>();
  for (auto const entity : light_view) {
    auto &transform = light_view.get<Transform>(entity);
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

  LevelData level_data{
    MOVE(gendata.tilegrid),
    MOVE(assets.tile_table),
    MOVE(gendata.startpos),
    MOVE(gendata.rivers),
    MOVE(gendata.torch_eid),

    assets.background_color,
    assets.global_light,
    MOVE(assets.obj_store),
    MOVE(camera),
    MOVE(player)
  };
  GfxState gfx{
    MOVE(assets.shader_programs),
    MOVE(assets.texture_table)
  };
  return ZoneState{
    MOVE(level_data),
    MOVE(gfx),
    registry};
}

void
bridge_staircases(ZoneState &a, ZoneState &b)
{
  auto &tilegrid_a = a.level_data.tilegrid();
  auto &tilegrid_b = b.level_data.tilegrid();

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
    auto const a_neighbors = find_immediate_neighbors(tilegrid_a, si_a.tile_position,
        TileType::FLOOR, behavior);
    assert(!a_neighbors.empty());

    auto const b_neighbors = find_immediate_neighbors(tilegrid_b, si_b.tile_position,
        TileType::FLOOR, behavior);
    assert(!b_neighbors.empty());

    // Set A's exit position to B's neighbor, and visa versa
    si_a.exit_position = b_neighbors.front();
    si_b.exit_position = a_neighbors.front();
  }
}

using copy_assets_pair_t = std::pair<EntityDrawHandles, TileDrawHandles>;
Result<copy_assets_pair_t, std::string>
copy_assets_gpu(stlw::Logger &logger, ShaderPrograms &sps, TileSharedInfoTable const& ttable,
    EntityRegistry &registry, ObjStore const &obj_store)
{
  EntityDrawinfos dinfos;

  // copy CUBES to GPU
  registry.view<ShaderName, CubeRenderable, PointLight>().each(
      [&](auto entity, auto &sn, auto &&...) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = opengl::gpu::copy_vertexonlycube_gpu(logger, shader_ref);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, CubeRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &, auto &texture) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto handle = opengl::gpu::copy_texturecube_gpu(logger, shader_ref, texture.texture_info);
        dinfos.add(entity, MOVE(handle));
      });

  // copy MESHES to GPU
  registry.view<ShaderName, MeshRenderable, Color>().each(
      [&](auto entity, auto &sn, auto &mesh, auto &) {
        auto &shader_ref = sps.ref_sp(sn.value);
        auto const va = shader_ref.va();
        auto const qa = QueryAttributes::from_va(va);
        auto const qo = ObjQuery{mesh.name, qa};
        auto const &obj = obj_store.get_obj(qo);

        auto handle = opengl::gpu::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, std::nullopt);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, TextureRenderable>().each(
      [&](auto entity, auto &sn, auto &mesh, auto &texture) {

        auto &shader_ref = sps.ref_sp(sn.value);
        auto const va = shader_ref.va();
        auto const qa = QueryAttributes::from_va(va);
        auto const qo = ObjQuery{mesh.name, qa};
        auto const &obj = obj_store.get_obj(qo);

        auto handle = opengl::gpu::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, texture.texture_info);
        dinfos.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, JunkEntityFromFILE>().each([&](auto entity, auto &sn, auto &mesh, auto &&...) {
    auto &shader_ref = sps.ref_sp(sn.value);
    auto const va = shader_ref.va();
    auto const qa = QueryAttributes::from_va(va);
    auto const qo = ObjQuery{mesh.name, qa};
    auto const &obj = obj_store.get_obj(qo);

    auto handle = opengl::gpu::copy_gpu(logger, GL_TRIANGLES, shader_ref, obj, std::nullopt);
    dinfos.add(entity, MOVE(handle));
  });

  // copy TILES to GPU
  std::vector<DrawInfo> tile_dinfos;
  tile_dinfos.reserve(static_cast<size_t>(TileType::UNDEFINED));
  for (auto const& it : ttable) {
    auto const& mesh_name = it.mesh_name;
    auto const& vshader_name = it.vshader_name;

        auto &shader_ref = sps.ref_sp(vshader_name);
        auto const va = shader_ref.va();
        auto const qa = QueryAttributes::from_va(va);
        auto const qo = ObjQuery{mesh_name, qa};
        auto const &obj = obj_store.get_obj(qo);

    auto handle = opengl::gpu::copy_gpu(logger, GL_TRIANGLES, sps.ref_sp(vshader_name), obj, std::nullopt);
    tile_dinfos[static_cast<size_t>(it.type)] = MOVE(handle);
  }

  EntityDrawHandles edh{MOVE(dinfos)};
  TileDrawHandles td{MOVE(tile_dinfos)};
  return Ok(std::make_pair(MOVE(edh), MOVE(td)));
}

void
copy_to_gpu(stlw::Logger &logger, ZoneState &zs)
{
  auto &ldata = zs.level_data;
  auto const& ttable = ldata.tiletable();
  auto const& objcache = ldata.obj_store;
  auto &gfx_state = zs.gfx_state;
  auto &sps = gfx_state.sps;
  auto &registry = zs.registry;

  auto copy_result = copy_assets_gpu(logger, sps, ttable, registry, objcache);
  assert(copy_result);
  auto handles = copy_result.expect_moveout("Error copying asset to gpu");
  auto edh = MOVE(handles.first);
  auto tdh = MOVE(handles.second);
  gfx_state.gpu_state.entities = MOVE(edh);
  gfx_state.gpu_state.tiles = MOVE(tdh);
}

} // ns anon

namespace boomhs
{

Result<ZoneStates, std::string>
LevelAssembler::assemble_levels(stlw::Logger &logger, std::vector<EntityRegistry> &registries)
{
  auto const level_string = [&](int const floor_number)
  {
    return "area" + std::to_string(floor_number) + ".toml";
  };

  auto const DUNGEON_FLOOR_COUNT = 2;
  std::vector<ZoneState> zstates;
  zstates.reserve(DUNGEON_FLOOR_COUNT + 1);

  stlw::float_generator rng;
  {
    // generate starting area
    auto &registry = registries[0];

    auto level_assets = TRY_MOVEOUT(LevelLoader::load_level(logger, registry, level_string(0)));
    auto gendata = StartAreaGenerator::gen_level(registry, rng, level_assets.texture_table);

    ZoneState zs = assemble(MOVE(gendata), MOVE(level_assets), registry);
    zstates.emplace_back(MOVE(zs));
  }

  auto const stairs_perfloor = 8;
  int const width = 40, height = 40;
  TileGridConfig const tdconfig{width, height};

  // TODO: it is currently not thread safe to call load_level() from multiple threads.
  //
  // The logger isn't thread safe, need to ensure that the logger isn't using "during" level gen,
  // or somehow give it unique access during writing (read/write lock?).
  for (auto i = 0; i < DUNGEON_FLOOR_COUNT; ++i) {
    auto &registry = registries[i + 1];
    auto level_assets = TRY_MOVEOUT(LevelLoader::load_level(logger, registry, level_string(i)));
    StairGenConfig const stairconfig{DUNGEON_FLOOR_COUNT, i, stairs_perfloor};
    LevelConfig const config{stairconfig, tdconfig};

    auto gendata = dungeon_generator::gen_level(config, registry, rng, level_assets.texture_table);

    ZoneState zs = assemble(MOVE(gendata), MOVE(level_assets), registry);
    zstates.emplace_back(MOVE(zs));
  }

  // insert a teleport tile on level1 for now (hacky)
  zstates[1].level_data.tilegrid().data(0, 0).type = TileType::TELEPORTER;

  // copy the first zonestate to GPU
  assert(zstates.size() > 0);
  copy_to_gpu(logger, zstates.front());
  copy_to_gpu(logger, zstates[1]);

  for(auto i = 2; i < DUNGEON_FLOOR_COUNT + 1; ++i) {
    bridge_staircases(zstates[i-1], zstates[i]);

    // TODO: maybe lazily load these?
    copy_to_gpu(logger, zstates[i]);
  }

  return OK_MOVE(zstates);
}

} // ns boomhs
