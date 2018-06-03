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

} // namespace

namespace boomhs
{

std::pair<EntityID, DrawInfo>
Tree::add_toregistry(stlw::Logger& logger, glm::vec3 const& world_pos, ObjStore const& obj_store,
                     ShaderPrograms& sps, EntityRegistry& registry)
{
  auto constexpr SN = "3d_pos_normal_color";
  auto constexpr TN = "tree_lowpoly";
  auto eid          = make_tree(world_pos, SN, TN, registry);

  auto&          va    = sps.ref_sp(SN).va();
  auto const     flags = BufferFlags::from_va(va);
  ObjQuery const query{TN, flags};

  auto& obj = obj_store.get(logger, TN);
  {
    auto& tc = registry.get<TreeComponent>(eid);
    tc.pobj  = &obj;
  }

  auto dinfo = opengl::gpu::copy_gpu(logger, va, obj);
  return std::make_pair(eid, MOVE(dinfo));
}

std::vector<float>
Tree::generate_tree_colors(stlw::Logger& logger, ObjData const& obj)
{
  auto const& materials     = obj.materials;
  auto const  get_facecolor = [&logger, &materials](auto const& shape,
                                                   auto const  f) { // per-face material
    int const face_materialid= shape.mesh.material_ids[f];
    // auto const& diffuse         = materials[face_materialid].diffuse;

    auto constdiffuse =
        face_materialid == 1 ? glm::vec3{0.5, 0.35, 0.05} : glm::vec3{0.0, 1.0, 0.3};
    return Color{diffuse[0], diffuse[1], diffuse[2], 1.0};
  };

  std::vector<float> colors;
  auto const         update_branchcolors = [&](auto const& shape, auto const& face) {
    auto const face_color = get_facecolor(shape, face);

    auto const fv = shape.mesh.num_face_vertices[face];
    FOR(vi, fv)
    {
      colors.emplace_back(face_color.r());
      colors.emplace_back(face_color.g());
      colors.emplace_back(face_color.b());
      colors.emplace_back(face_color.a());
    }
  };
  obj.foreach_face(update_branchcolors);

  assert((obj.colors.size() % 4) == 0);
  assert(obj.colors.size() == colors.size());

  return colors;
}

} // namespace boomhs
