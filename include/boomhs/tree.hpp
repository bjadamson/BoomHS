#pragma once
#include <opengl/draw_info.hpp>
#include <stlw/log.hpp>

#include <extlibs/glm.hpp>
#include <array>
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

struct TreeComponent
{
  ObjData*                     pobj;
  std::array<opengl::Color, 4> colors;

  TreeComponent();
  COPYMOVE_DEFAULT(TreeComponent);
};

class Tree
{
  Tree() = delete;

public:
  static void
  update_colors(stlw::Logger&, opengl::VertexAttribute const&, opengl::DrawInfo&, TreeComponent&);

  static std::pair<EntityID, opengl::DrawInfo>
  add_toregistry(stlw::Logger&, EntityID, ObjStore&, opengl::ShaderPrograms&, EntityRegistry&);
};

} // namespace boomhs
