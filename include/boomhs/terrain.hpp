#pragma once
#include <extlibs/glm.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/heightmap.hpp>

#include <stlw/log.hpp>

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace boomhs
{

class Terrain
{
  glm::vec2           pos_;
  opengl::DrawInfo    di_;
  opengl::TextureInfo ti_;

public:
  static int const SIZE;

  // The number of vertices along an edge of the terrain.
  static int const VERTEX_COUNT;

  Terrain(glm::vec2 const&, opengl::DrawInfo&&, opengl::TextureInfo const&);

  auto const& position() const { return pos_; }
  auto const& draw_info() const { return di_; }
  auto const& texture_info() const { return ti_; }
};

namespace terrain
{

Terrain
generate(stlw::Logger&, glm::vec2 const&, opengl::HeightmapData const&, opengl::ShaderProgram&,
    opengl::TextureInfo const&);

} // namespace terrain

} // namespace boomhs
