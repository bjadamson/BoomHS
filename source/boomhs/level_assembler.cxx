#include <boomhs/billboard.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/level_assembler.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/tree.hpp>

#include <boomhs/obj.hpp>
#include <opengl/gpu.hpp>
#include <opengl/factory.hpp>
#include <boomhs/lighting.hpp>

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
  registry.view<ShaderName, CubeRenderable, PointLight>().each(
      [&](auto const eid, auto& sn, auto& cr, auto&&... args) {
      copy_cube(eid, sn, cr, FORWARD(args));
      });
  registry.view<ShaderName, CubeRenderable, TextureRenderable>().each(
      [&](auto const eid, auto& sn, auto& cr, auto& texture) {
        copy_cube(eid, sn, cr, texture);
      });

  // copy MESHES to GPU
  registry.view<ShaderName, MeshRenderable, Color>().each(
      [&](auto entity, auto& sn, auto& mesh, auto&) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get(logger, mesh.name);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        draw_handles.add_entity(entity, MOVE(handle));
      });
  registry.view<ShaderName, MeshRenderable, TextureRenderable>().each(
      [&](auto entity, auto& sn, auto& mesh, auto& texture) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get(logger, mesh.name);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        draw_handles.add_entity(entity, MOVE(handle));
      });
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
  registry.view<ShaderName, MeshRenderable, JunkEntityFromFILE>().each(
      [&](auto entity, auto& sn, auto& mesh, auto&&...) {
        auto&       va  = sps.ref_sp(sn.value).va();
        auto const  qa  = BufferFlags::from_va(va);
        auto const  qo  = ObjQuery{mesh.name, qa};
        auto const& obj = obj_store.get(logger, mesh.name);

        auto handle = opengl::gpu::copy_gpu(logger, va, obj);
        draw_handles.add_entity(entity, MOVE(handle));
      });

  registry.view<ShaderName, MeshRenderable, TreeComponent>().each(
      [&](auto entity, auto& sn, auto& mesh, auto& tree) {
        auto& name = registry.get<MeshRenderable>(entity).name;

        auto&          va    = sps.ref_sp(sn.value).va();
        auto const     flags = BufferFlags::from_va(va);
        ObjQuery const query{name, flags};
        auto&          obj = obj_store.get(logger, name);

        auto& tc = registry.get<typename std::remove_reference<decltype(tree)>::type>(entity);
        tc.set_obj(&obj);

        auto& dinfo = draw_handles.lookup_entity(logger, entity);
        Tree::update_colors(logger, va, dinfo, tc);
      });

  return OK_NONE;
}

void
copy_to_gpu(stlw::Logger& logger, ZoneState& zs)
{
  auto&       ldata     = zs.level_data;
  auto&       objstore  = ldata.obj_store;
  auto&       gfx_state = zs.gfx_state;
  auto&       sps       = gfx_state.sps;
  auto&       registry  = zs.registry;

  auto& draw_handles = gfx_state.draw_handles;
  auto copy_result = copy_assets_gpu(logger, sps, registry, objstore, draw_handles)
    .expect_moveout("Error copying asset to gpu");

  auto& obj_store                 = ldata.obj_store;
  auto constexpr WIREFRAME_SHADER = "wireframe";
  auto& va                        = sps.ref_sp(WIREFRAME_SHADER).va();

  auto const          add_wireframe = [&](auto const eid, auto const& min, auto const& max) {
    {
      auto& bbox = registry.assign<AABoundingBox>(eid);
      bbox.min   = min;
      bbox.max   = max;

      CubeMinMax const cmm{bbox.min, bbox.max};
      auto const cv = OF::cube_vertices(cmm.min, cmm.max);
      auto    dinfo = opengl::gpu::copy_cube_wireframe_gpu(logger, cv, va);
      draw_handles.add_bbox(eid, MOVE(dinfo));
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

    add_wireframe(eid, min, max);
    registry.assign<Selectable>(eid);
  }
  for (auto const eid : registry.view<CubeRenderable>()) {
    auto const& cr = registry.get<CubeRenderable>(eid);

    add_wireframe(eid, cr.min, cr.max);
    registry.assign<Selectable>(eid);
  }
  for (auto const eid : registry.view<OrbitalBody>()) {
    glm::vec3 constexpr min = glm::vec3{0.0};
    glm::vec3 constexpr max = glm::vec3{0.0};
    add_wireframe(eid, min, max);
  }
}

} // namespace

namespace boomhs
{

Result<ZoneStates, std::string>
LevelAssembler::assemble_levels(stlw::Logger& logger, std::vector<EntityRegistry>& registries)
{
  auto const level_string = [&](int const floor_number) -> std::string {
    return "area" + std::to_string(floor_number) + ".toml";
  };

  std::vector<ZoneState> zstates;
  zstates.reserve(1);

  stlw::float_generator rng;
  {
    // generate starting area
    int constexpr FLOOR_NUMBER = 0;
    auto& registry = registries[FLOOR_NUMBER];

    auto  level_name     = level_string(FLOOR_NUMBER);
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

  // copy the first zonestate to GPU
  assert(zstates.size() > 0);
  copy_to_gpu(logger, zstates.front());

  return OK_MOVE(zstates);
}

} // namespace boomhs
