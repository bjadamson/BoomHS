#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/obj_store.hpp>
#include <boomhs/tree.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto
make_tree(glm::vec3 const& world_pos, char const* shader_name, char const* tree_name,
          EntityRegistry& registry)
{
  auto  eid = registry.create();
  auto& mr  = registry.assign<MeshRenderable>(eid);
  mr.name   = tree_name;

  auto& transform       = registry.assign<Transform>(eid);
  transform.translation = world_pos;
  registry.assign<Material>(eid);
  // registry.assign<JunkEntityFromFILE>(eid);

  auto& tc = registry.assign<TreeComponent>(eid);
  {
    auto& isv = registry.assign<IsVisible>(eid);
    isv.value = true;
  }
  registry.assign<Name>(eid).value = tree_name;

  auto& sn = registry.assign<ShaderName>(eid);
  sn.value = shader_name;
  {
    auto& cc = registry.assign<Color>(eid);
    *&cc     = LOC::WHITE;
  }

  return eid;
}

std::vector<float>
generate_tree_colors(stlw::Logger& logger, TreeComponent const& tc)
{
  std::vector<float> colors;
  auto const         update_branchcolors = [&](auto const& shape, int const face) {
    int const   face_materialid = shape.mesh.material_ids[face];
    auto const& face_color      = tc.colors[face_materialid];

    auto const fv = shape.mesh.num_face_vertices[face];
    FOR(vi, fv)
    {
      colors.emplace_back(face_color.r());
      colors.emplace_back(face_color.g());
      colors.emplace_back(face_color.b());
      colors.emplace_back(face_color.a());
    }
  };

  assert(tc.pobj);
  auto const& obj = *tc.pobj;
  obj.foreach_face(update_branchcolors);

  assert((obj.colors.size() % 4) == 0);
  assert(obj.colors.size() == colors.size());

  return colors;
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// TreeComponent
TreeComponent::TreeComponent()
    : pobj(nullptr)
    , colors{LOC::GREEN, LOC::YELLOW, LOC::BROWN}
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Tree
void
Tree::update_colors(stlw::Logger& logger, VertexAttribute const& va, DrawInfo& dinfo,
                    TreeComponent& tc)
{
  assert(nullptr != tc.pobj);
  auto& objdata  = *tc.pobj;
  objdata.colors = generate_tree_colors(logger, tc);
  gpu::overwrite_vertex_buffer(logger, va, dinfo, objdata);
}

std::pair<EntityID, DrawInfo>
Tree::add_toregistry(stlw::Logger& logger, EntityID const eid, ObjStore& obj_store,
                     ShaderPrograms& sps, EntityRegistry& registry)
{
  auto& sn     = registry.get<ShaderName>(eid).value;
  auto&  va    = sps.ref_sp(sn).va();

  auto& mesh   = registry.get<MeshRenderable>(eid);
  ObjData& obj = obj_store.get(logger, mesh.name);

  auto dinfo = opengl::gpu::copy_gpu(logger, va, obj);
  return std::make_pair(eid, MOVE(dinfo));
}

} // namespace boomhs
