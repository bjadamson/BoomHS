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

template <typename FN>
std::vector<float>
generate_tree_colors(common::Logger& logger, ObjData const& objdata, FN const& face_to_colormap)
{
  std::vector<float> colors;
  auto const         update_branchcolors = [&](auto const& shape, int const face) {
    int const face_materialid = shape.mesh.material_ids[face];

    assert(static_cast<size_t>(face_materialid) < face_to_colormap.size());
    auto const& face_color = *face_to_colormap[face_materialid];

    auto const fv = shape.mesh.num_face_vertices[face];
    FOR(vi, fv)
    {
      colors.emplace_back(face_color.r());
      colors.emplace_back(face_color.g());
      colors.emplace_back(face_color.b());
      colors.emplace_back(face_color.a());
    }
  };

  objdata.foreach_face(update_branchcolors);

  assert((objdata.colors.size() % 4) == 0);
  assert(objdata.colors.size() == colors.size());

  return colors;
}

std::vector<float>
generate_tree_colors(common::Logger& logger, TreeComponent const& tree)
{
  std::vector<Color const*> face_to_colormap;

  FOR(i, tree.num_leaves()) { face_to_colormap.emplace_back(&tree.leaf_color(i)); }
  FOR(i, tree.num_stems()) { face_to_colormap.emplace_back(&tree.stem_color(i)); }
  FOR(i, tree.num_trunks()) { face_to_colormap.emplace_back(&tree.trunk_color(i)); }
  return generate_tree_colors(logger, tree.obj(), face_to_colormap);
}

size_t
leaves_offset(TreeComponent const& tc)
{
  // Leaves are stored at the beginning
  return 0;
}

auto
stem_offset(TreeComponent const& tc)
{
  // Stems come after the leaves.
  return tc.num_leaves();
}

auto
trunk_offset(TreeComponent const& tc)
{
  // Trunks come after the leaves and stems.
  return tc.num_leaves() + tc.num_stems();
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// TreeComponent
TreeComponent::TreeComponent()
    : pobj_{nullptr}
    , num_trunks_(0)
    , num_stems_(0)
    , num_leaves_(0)
{
}

void
TreeComponent::assert_onemore_isnot_toomany() const
{
  auto const num_colors = num_trunks_ + num_stems_ + num_leaves_;
  assert((num_colors + 1) <= TreeComponent::MAX_NUMBER_TREE_COLORS);
}

ObjData&
TreeComponent::obj()
{
  assert(nullptr != pobj_);
  return *pobj_;
}

ObjData const&
TreeComponent::obj() const
{
  assert(nullptr != pobj_);
  return *pobj_;
}

void
TreeComponent::set_obj(ObjData* p)
{
  assert(nullptr != p);
  pobj_ = p;
}

void
TreeComponent::add_color(TreeColorType const type, opengl::Color const& color)
{
  assert_onemore_isnot_toomany();

  size_t offset = 0;
  switch (type) {
  case TreeColorType::Trunk:
    offset = trunk_offset(*this) + num_trunks_;
    ++num_trunks_;
    break;
  case TreeColorType::Stem:
    offset = stem_offset(*this) + num_stems_;
    ++num_stems_;
    break;
  case TreeColorType::Leaf:
    offset = leaves_offset(*this) + num_leaves_;
    ++num_leaves_;
    break;
  default:
    std::abort();
  }

  assert(offset < colors.size());
  colors[offset] = color;
}

#define COLOR_FROM_INDEX(INDEX, MAX_NUM_INTERNAL, OFFSET, THIS)                                    \
  assert(index < MAX_NUM_INTERNAL);                                                                \
  return THIS->colors[OFFSET + index];

Color const&
TreeComponent::trunk_color(size_t const index) const
{
  COLOR_FROM_INDEX(index, num_trunks(), trunk_offset(*this), this);
}

Color&
TreeComponent::trunk_color(size_t const index)
{
  COLOR_FROM_INDEX(index, num_trunks(), trunk_offset(*this), this);
}

Color const&
TreeComponent::stem_color(size_t const index) const
{
  COLOR_FROM_INDEX(index, num_stems(), stem_offset(*this), this);
}

Color&
TreeComponent::stem_color(size_t const index)
{
  COLOR_FROM_INDEX(index, num_stems(), stem_offset(*this), this);
}

Color&
TreeComponent::leaf_color(size_t const index)
{
  COLOR_FROM_INDEX(index, num_leaves(), leaves_offset(*this), this);
}

Color const&
TreeComponent::leaf_color(size_t const index) const
{
  COLOR_FROM_INDEX(index, num_leaves(), leaves_offset(*this), this);
}

#undef COLOR_FROM_INDEX

///////////////////////////////////////////////////////////////////////////////////////////////////
// Tree
void
Tree::update_colors(common::Logger& logger, VertexAttribute const& va, DrawInfo& dinfo,
                    TreeComponent& tree)
{
  auto& objdata  = tree.obj();
  objdata.colors = generate_tree_colors(logger, tree);
  gpu::overwrite_vertex_buffer(logger, va, dinfo, objdata);
}

/*
std::pair<EntityID, DrawInfo>
Tree::add_toregistry(common::Logger& logger, EntityID const eid, ObjStore& obj_store,
                     ShaderPrograms& sps, EntityRegistry& registry)
{
  auto& sn = registry.get<ShaderName>(eid).value;
  auto& va = sps.ref_sp(sn).va();

  auto&    mesh = registry.get<MeshRenderable>(eid);
  ObjData& obj  = obj_store.get(logger, mesh.name);

  auto dinfo = opengl::gpu::copy_gpu(logger, va, obj);
  return std::make_pair(eid, MOVE(dinfo));
}
*/

} // namespace boomhs
