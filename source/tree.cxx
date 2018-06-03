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

  registry.assign<TreeComponent>(eid);
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

  auto& obj   = obj_store.get(logger, TN);
  auto  dinfo = opengl::gpu::copy_gpu(logger, va, obj);
  return std::make_pair(eid, MOVE(dinfo));
}

} // namespace boomhs
