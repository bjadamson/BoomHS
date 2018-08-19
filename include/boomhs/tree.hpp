#pragma once
#include <common/log.hpp>
#include <opengl/draw_info.hpp>

#include <array>
#include <extlibs/glm.hpp>
#include <utility>

namespace opengl
{
class ShaderPrograms;
} // namespace opengl

namespace boomhs
{
class ObjData;
class ObjStore;
class EntityRegistry;

template <size_t N>
using TreeColors = std::array<Color, N>;

enum class TreeColorType
{
  Trunk = 0,
  Stem,
  Leaf
};

class TreeComponent
{
  ObjData* pobj_;

  size_t num_trunks_;
  size_t num_stems_;
  size_t num_leaves_;

  void assert_onemore_isnot_toomany() const;

public:
  static constexpr int               MAX_NUMBER_TREE_COLORS = 4;
  TreeColors<MAX_NUMBER_TREE_COLORS> colors;

  ObjData&       obj();
  ObjData const& obj() const;

  void add_color(TreeColorType, Color const&);

  Color& trunk_color(size_t);
  Color& stem_color(size_t);
  Color& leaf_color(size_t);

  Color const& trunk_color(size_t) const;
  Color const& stem_color(size_t) const;
  Color const& leaf_color(size_t) const;

  auto num_trunks() const { return num_trunks_; }
  auto num_stems() const { return num_stems_; }
  auto num_leaves() const { return num_leaves_; }

  TreeComponent(ObjData&);
  COPYMOVE_DEFAULT(TreeComponent);
};

class Tree
{
  Tree() = delete;

public:
  static void
  update_colors(common::Logger&, opengl::VertexAttribute const&, opengl::DrawInfo&, TreeComponent&);

  // static std::pair<EntityID, opengl::DrawInfo>
  // add_toregistry(common::Logger&, EntityID, ObjStore&, opengl::ShaderPrograms&, EntityRegistry&);
};

} // namespace boomhs
