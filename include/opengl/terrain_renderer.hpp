#pragma once
#include <opengl/bind.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <string>

namespace boomhs
{
class EntityRegistry;
struct RenderState;
class Terrain;
} // namespace boomhs

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{
class ShaderProgram;
class TextureTable;

class BasicTerrainRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(BasicTerrainRenderer);
  BasicTerrainRenderer() = default;

  // fields
  opengl::DebugBoundCheck debug_check;

  // methods
  void render(boomhs::RenderState&, boomhs::EntityRegistry& registry, window::FrameTime const&,
              glm::vec4 const&);

  void bind_impl(stlw::Logger&, boomhs::Terrain const&, opengl::TextureTable&);
  void unbind_impl(stlw::Logger&, boomhs::Terrain const&, opengl::TextureTable&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  std::string to_string() const;
};

class BlackTerrainRenderer
{
  opengl::ShaderProgram& sp_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(BlackTerrainRenderer);
  BlackTerrainRenderer(opengl::ShaderProgram&);

  // methods
  void
  render(boomhs::RenderState&, boomhs::EntityRegistry&, window::FrameTime const&, glm::vec4 const&);
};

} // namespace opengl
