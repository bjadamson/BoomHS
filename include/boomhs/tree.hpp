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
  ObjData const* pobj = nullptr;
};

class Tree
{
public:
  static std::pair<EntityID, opengl::DrawInfo>
  add_toregistry(stlw::Logger&, glm::vec3 const&, ObjStore const&, opengl::ShaderPrograms&,
                 EntityRegistry&);

  static std::vector<float> generate_tree_colors(stlw::Logger&, ObjData const&);
};

} // namespace boomhs
