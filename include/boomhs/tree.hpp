#pragma once
#include <opengl/draw_info.hpp>
#include <stlw/log.hpp>

#include <extlibs/glm.hpp>
#include <utility>

namespace opengl
{
class ShaderPrograms;
} // ns opengl

namespace boomhs
{
class ObjStore;
class EntityRegistry;

struct TreeComponent
{
};

class Tree
{
public:
  static std::pair<EntityID, opengl::DrawInfo>
  add_toregistry(stlw::Logger&, glm::vec3 const&, ObjStore const&,
      opengl::ShaderPrograms&, EntityRegistry&);
};

} // ns boomhs
