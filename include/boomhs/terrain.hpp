#pragma once
#include <extlibs/glm.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/heightmap.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

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
  Terrain(glm::vec2 const&, opengl::DrawInfo&&, opengl::TextureInfo const&);

  auto const& position() const { return pos_; }
  auto const& draw_info() const { return di_; }
  auto const& texture_info() const { return ti_; }
};

class TerrainGrid
{
  float const          grid_size_;
  std::vector<Terrain> terrains_;

public:
  explicit TerrainGrid(float);

  void add(Terrain&&);

  MOVE_CONSTRUCTIBLE_ONLY(TerrainGrid);
  BEGIN_END_FORWARD_FNS(terrains_);

  auto height() const { return grid_size_; }
  auto width() const { return grid_size_; }

  auto count() const { return terrains_.size(); }
};

namespace terrain
{

Terrain
generate(stlw::Logger&, glm::vec2 const&, float, opengl::HeightmapData const&,
         opengl::ShaderProgram&, opengl::TextureInfo const&);

} // namespace terrain

} // namespace boomhs
