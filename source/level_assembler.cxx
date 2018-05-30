#include <boomhs/billboard.hpp>
#include <boomhs/components.hpp>
#include <boomhs/dungeon_generator.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/player.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/tilegrid_algorithms.hpp>
#include <boomhs/world_object.hpp>

#include <boomhs/obj.hpp>
#include <opengl/constants.hpp>
#include <opengl/gpu.hpp>
#include <opengl/lighting.hpp>

#include <sstream>
#include <stlw/os.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

ZoneState
assemble(LevelGeneratedData&& gendata, LevelAssets&& assets, EntityRegistry& registry)
{
  auto const player_eid = find_player(registry);
  EnttLookup player_lookup{player_eid, registry};

  auto const FORWARD = -Z_UNIT_VECTOR;
  auto constexpr UP  = Y_UNIT_VECTOR;
  WorldObject player{player_lookup, FORWARD, UP};

  // Combine the generated data with the asset data, creating the LevelData instance.
  LevelData level_data{MOVE(gendata.tilegrid),
                       MOVE(assets.tile_table),
                       MOVE(gendata.startpos),
                       MOVE(gendata.rivers),
                       MOVE(gendata.terrain),
                       MOVE(gendata.water),

                       assets.fog,
                       assets.global_light,
                       MOVE(assets.obj_store),
                       MOVE(player)};
  GfxState  gfx{MOVE(assets.shader_programs), MOVE(assets.texture_table)};
  return ZoneState{MOVE(level_data), MOVE(gfx), registry};
}

void
bridge_staircases(ZoneState& a, ZoneState& b)
{
  auto& tilegrid_a = a.level_data.tilegrid();
  auto& tilegrid_b = b.level_data.tilegrid();

  auto const stairs_up_a = find_upstairs(a.registry, tilegrid_a);
  assert(!stairs_up_a.empty());

  auto const stairs_down_b = find_downstairs(b.registry, tilegrid_b);
  assert(!stairs_down_b.empty());

  auto& a_registry = a.registry;
  auto& b_registry = b.registry;

  auto const behavior = TileLookupBehavior::VERTICAL_HORIZONTAL_ONLY;

  assert(stairs_up_a.size() == stairs_down_b.size());
  auto const num_stairs = stairs_up_a.size();
  FOR(i, num_stairs)
  {
    auto const a_updowneid = stairs_up_a[i];
    auto const b_downeid   = stairs_down_b[i];

    assert(a_registry.has<StairInfo>(a_updowneid));
    StairInfo& si_a = a_registry.get<StairInfo>(a_updowneid);

    assert(b_registry.has<StairInfo>(b_downeid));
    StairInfo& si_b = b_registry.get<StairInfo>(b_downeid);

    // find a suitable neighbor tile for each stair
    auto const a_neighbors =
        find_immediate_neighbors(tilegrid_a, si_a.tile_position, TileType::FLOOR, behavior);
    assert(!a_neighbors.empty());

    auto const b_neighbors =
        find_immediate_neighbors(tilegrid_b, si_b.tile_position, TileType::FLOOR, behavior);
    assert(!b_neighbors.empty());

    // Set A's exit position to B's neighbor, and visa versa
    si_a.exit_position = b_neighbors.front();
    si_b.exit_position = a_neighbors.front();
  }
}

using copy_assets_pair_t = std::pair<EntityDrawHandleMap, TileDrawHandles>;
Result<copy_assets_pair_t, std::string>
copy_assets_gpu(stlw::Logger& logger, ShaderPrograms& sps, TileSharedInfoTable const& ttable,
                EntityRegistry& registry, ObjStore const& obj_store)
{
  EntityDrawHandleMap entity_drawmap;

  // copy CUBES to GPU
  registry.view<ShaderName, CubeRenderable, PointLight>().each(
      [&](auto entity, auto& sn, auto&&...) {
        auto& va     = sps.ref_sp(sn.value).va();
        auto  handle = opengl::gpu::copy_cubevertexonly_gpu(logger, va);
        entity_drawmap.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, CubeRenderable, TextureRenderable>().each(
      [&](auto entity, auto& sn, auto&, auto& texture) {
        auto& va     = sps.ref_sp(sn.value).va();
        auto  handle = opengl::gpu::copy_cubetexture_gpu(logger, va);
        entity_drawmap.add(entity, MOVE(handle));
      });

  // copy MESHES to GPU
  registry.view<ShaderName, MeshRenderable, Color>().each(
      [&](auto entity, auto& sn, auto& mesh, auto&) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get_obj(logger, qo);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        entity_drawmap.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, TextureRenderable>().each(
      [&](auto entity, auto& sn, auto& mesh, auto& texture) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get_obj(logger, qo);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        entity_drawmap.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, BillboardRenderable, TextureRenderable>().each(
      [&](auto entity, auto& sn, auto&, auto& texture) {
        auto&      va = sps.ref_sp(sn.value).va();
        auto const v  = OF::rectangle_vertices();
        auto*      ti = texture.texture_info;
        assert(ti);
        auto handle = opengl::gpu::copy_rectangle_uvs(logger, va, v, *ti);
        entity_drawmap.add(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, JunkEntityFromFILE>().each(
      [&](auto entity, auto& sn, auto& mesh, auto&&...) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get_obj(logger, qo);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        entity_drawmap.add(entity, MOVE(handle));
      });

  // copy TILES to GPU
  std::vector<DrawInfo> tile_dinfos;
  tile_dinfos.reserve(static_cast<size_t>(TileType::UNDEFINED));
  for (auto const& it : ttable) {
    auto const& mesh_name    = it.mesh_name;
    auto const& vshader_name = it.vshader_name;

    auto&       va  = sps.ref_sp(vshader_name).va();
    auto const  qa  = BufferFlags::from_va(va);
    auto const  qo  = ObjQuery{mesh_name, qa};
    auto const& obj = obj_store.get_obj(logger, qo);

    auto handle = opengl::gpu::copy_gpu(logger, sps.ref_sp(vshader_name).va(), obj);

    assert(it.type < TileType::UNDEFINED);
    auto const index = static_cast<size_t>(it.type);
    assert(index < tile_dinfos.capacity());
    tile_dinfos[index] = MOVE(handle);
  }

  TileDrawHandles td{MOVE(tile_dinfos)};
  return Ok(std::make_pair(MOVE(entity_drawmap), MOVE(td)));
}

struct TreeComponent
{
};

void
copy_to_gpu(stlw::Logger& logger, ZoneState& zs)
{
  auto&       ldata     = zs.level_data;
  auto const& ttable    = ldata.tiletable();
  auto const& objcache  = ldata.obj_store;
  auto&       gfx_state = zs.gfx_state;
  auto&       sps       = gfx_state.sps;
  auto&       registry  = zs.registry;

  auto copy_result = copy_assets_gpu(logger, sps, ttable, registry, objcache);
  assert(copy_result);
  auto handles = copy_result.expect_moveout("Error copying asset to gpu");
  auto edh     = MOVE(handles.first);
  auto tdh     = MOVE(handles.second);

  auto const& obj_store = ldata.obj_store;

  auto const add_tree = [&](auto const& world_pos) -> Transform& {
    auto  eid = registry.create();
    auto& mr  = registry.assign<MeshRenderable>(eid);
    mr.name   = "sphere";

    auto& transform       = registry.assign<Transform>(eid);
    transform.translation = world_pos;
    registry.assign<Material>(eid);
    registry.assign<JunkEntityFromFILE>(eid);
    //registry.assign<TreeComponent>(eid);
    {
      auto& isv = registry.assign<IsVisible>(eid);
      isv.value = true;
    }
    registry.assign<Name>(eid).value = "custom tree";

    auto& sn = registry.assign<ShaderName>(eid);
    sn.value = "3d_pos_normal_color";
    {
      auto& cc = registry.assign<Color>(eid);
      *&cc     = LOC::WHITE;
    }

    auto&          va    = sps.ref_sp(sn.value).va();
    auto const     flags = BufferFlags::from_va(va);
    ObjQuery const query{mr.name, flags};
    auto&          obj   = obj_store.get_obj(logger, query);
    auto           dinfo = opengl::gpu::copy_gpu(logger, va, obj);
    edh.add(eid, MOVE(dinfo));

    return transform;
  };
             add_tree(glm::vec3{2.0f, 0.0f, 0.0f});
             add_tree(glm::vec3{0.0f, 0.0f, 2.0f});
  auto* tr = &add_tree(glm::vec3{4.0f, 4.0f, 0.0f});
  tr->scale = glm::vec3{2.0f};


  tr = &add_tree(glm::vec3{100.0f, 10.0f, 100.0f});
  tr->scale = glm::vec3{60.0f};

  EntityDrawHandleMap bbox_dh;
  for (auto const eid : registry.view<MeshRenderable>()) {
    {
      registry.assign<Selectable>(eid);
      auto& name = registry.get<MeshRenderable>(eid).name;

      auto constexpr WIREFRAME_SHADER = "wireframe";
      auto& va                        = sps.ref_sp(WIREFRAME_SHADER).va();
      {
        auto const     flags = BufferFlags::from_va(va);
        ObjQuery const query{name, flags};
        auto&          obj  = obj_store.get_obj(logger, query);
        auto&          bbox = registry.assign<AABoundingBox>(eid);

        auto const posbuffer = obj.positions();
        bbox.min             = posbuffer.min();
        bbox.max             = posbuffer.max();

        // LOG_ERROR_SPRINTF("box: [min: %s, max: %s, midp: %s], ", glm::to_string(bbox.min),
        // glm::to_string(bbox.max), glm::to_string(bbox.max - bbox.min));

        auto dinfo = opengl::gpu::copy_cube_wireframevertexonly_gpu(logger, va, bbox.min, bbox.max);
        bbox_dh.add(eid, MOVE(dinfo));
      }
    }
  }

  auto& gpu_state                = gfx_state.gpu_state;
  gpu_state.entities             = MOVE(edh);
  gpu_state.entity_boundingboxes = MOVE(bbox_dh);
  gpu_state.tiles                = MOVE(tdh);
}

} // namespace

namespace boomhs
{

Result<ZoneStates, std::string>
LevelAssembler::assemble_levels(stlw::Logger& logger, std::vector<EntityRegistry>& registries)
{
  auto const level_string = [&](int const floor_number) {
    return "area" + std::to_string(floor_number) + ".toml";
  };

  auto const             DUNGEON_FLOOR_COUNT = 2;
  std::vector<ZoneState> zstates;
  zstates.reserve(DUNGEON_FLOOR_COUNT + 1);

  stlw::float_generator rng;
  {
    // generate starting area
    auto& registry = registries[0];

    auto  level_assets = TRY_MOVEOUT(LevelLoader::load_level(logger, registry, level_string(0)));
    auto& ttable       = level_assets.texture_table;
    auto& sps          = level_assets.shader_programs;
    auto  gendata      = StartAreaGenerator::gen_level(logger, registry, rng, sps, ttable);

    ZoneState zs = assemble(MOVE(gendata), MOVE(level_assets), registry);
    zstates.emplace_back(MOVE(zs));
  }

  auto const           stairs_perfloor = 8;
  int const            width = 40, height = 40;
  TileGridConfig const tdconfig{width, height};

  // TODO: it is currently not thread safe to call load_level() from multiple threads.
  //
  // The logger isn't thread safe, need to ensure that the logger isn't using "during" level gen,
  // or somehow give it unique access during writing (read/write lock?).
  for (auto i = 0; i < DUNGEON_FLOOR_COUNT; ++i) {
    auto& registry     = registries[i + 1];
    auto  level_assets = TRY_MOVEOUT(LevelLoader::load_level(logger, registry, level_string(i)));
    StairGenConfig const stairconfig{DUNGEON_FLOOR_COUNT, i, stairs_perfloor};
    LevelConfig const    config{stairconfig, tdconfig};

    auto& ttable  = level_assets.texture_table;
    auto& sps     = level_assets.shader_programs;
    auto  gendata = dungeon_generator::gen_level(logger, config, registry, rng, sps, ttable);

    ZoneState zs = assemble(MOVE(gendata), MOVE(level_assets), registry);
    zstates.emplace_back(MOVE(zs));
  }

  // insert a teleport tile on level1 for now (hacky)
  zstates[1].level_data.tilegrid().data(0, 0).type = TileType::TELEPORTER;

  // copy the first zonestate to GPU
  assert(zstates.size() > 0);
  copy_to_gpu(logger, zstates.front());
  copy_to_gpu(logger, zstates[1]);

  for (auto i = 2; i < DUNGEON_FLOOR_COUNT + 1; ++i) {
    bridge_staircases(zstates[i - 1], zstates[i]);

    // TODO: maybe lazily load these?
    copy_to_gpu(logger, zstates[i]);
  }

  return OK_MOVE(zstates);
}

} // namespace boomhs
