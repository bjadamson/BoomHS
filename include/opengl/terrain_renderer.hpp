#pragma once
#include <opengl/bind.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <string>

namespace boomhs
{
class EntityRegistry;
class MaterialTable;
class Terrain;
} // namespace boomhs

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{
struct RenderState;
class ShaderProgram;
class TextureTable;

class DefaultTerrainRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(DefaultTerrainRenderer);
  DefaultTerrainRenderer() = default;

  // fields
  opengl::DebugBoundCheck debug_check;

  // methods
  void render(RenderState&, boomhs::MaterialTable const&, boomhs::EntityRegistry& registry,
              window::FrameTime const&, glm::vec4 const&);

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
  void render(RenderState&, boomhs::MaterialTable const&, boomhs::EntityRegistry&,
              window::FrameTime const&, glm::vec4 const&);
};

} // namespace opengl
