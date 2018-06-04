#pragma once
#include <opengl/draw_info.hpp>
#include <stlw/log.hpp>

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

struct TreeComponent
{
  ObjData*      pobj      = nullptr;
  opengl::Color colors[3] = {LOC::GREEN, LOC::YELLOW, LOC::BROWN};

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
  add_toregistry(stlw::Logger&, EntityID, ObjStore&, opengl::ShaderPrograms&,
                 EntityRegistry&);
};

} // namespace boomhs
