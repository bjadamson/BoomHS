#pragma once
#include <extlibs/glm.hpp>

namespace boomhs
{
struct Material;
class  EntityRegistry;
} // namespace boomhs

namespace opengl
{
struct RenderState;
class  ShaderProgram;

class LightRenderer
{
  LightRenderer() = delete;
public:

  static void
  set_light_uniforms(RenderState&, boomhs::EntityRegistry&, ShaderProgram&,
                     boomhs::Material const&, glm::vec3 const&, glm::mat4 const&, bool);
};

} // namespace opengl
