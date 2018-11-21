#pragma once
#include <opengl/bind.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <string>

namespace boomhs
{
class  EntityRegistry;
class  FrameTime;
struct MaterialTable;
class  Terrain;
} // namespace boomhs

namespace opengl
{
struct RenderState;
class ShaderProgram;
class TextureTable;

class DefaultTerrainRenderer
{
public:
  NOCOPY_MOVE_DEFAULT(DefaultTerrainRenderer);
  DefaultTerrainRenderer() = default;

  // fields
  opengl::DebugBoundCheck debug_check;

  // methods
  void render(RenderState&, boomhs::MaterialTable const&, boomhs::EntityRegistry& registry,
              boomhs::FrameTime const&, glm::vec4 const&);

  void bind_impl(common::Logger&, boomhs::Terrain const&, opengl::TextureTable&);
  void unbind_impl(common::Logger&, boomhs::Terrain const&, opengl::TextureTable&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  std::string to_string() const;
};

class SilhouetteTerrainRenderer
{
  opengl::ShaderProgram* sp_;

public:
  NOCOPY_MOVE_DEFAULT(SilhouetteTerrainRenderer);
  SilhouetteTerrainRenderer(opengl::ShaderProgram&);

  // methods
  void render(RenderState&, boomhs::MaterialTable const&, boomhs::EntityRegistry&,
              boomhs::FrameTime const&, glm::vec4 const&);
};

} // namespace opengl
