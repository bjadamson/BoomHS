#pragma once
#include <extlibs/glm.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/heightmap.hpp>

#include <ostream>
#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace boomhs
{

struct TerrainPieceConfig
{
  TerrainPieceConfig();

  size_t num_vertexes;
  float  height_multiplier;
  bool   invert_normals;

  GLint wrap_mode   = GL_REPEAT;
  float uv_max      = 1.0f;
  float uv_modifier = 1.0f;

  std::string shader_name;
  std::string texture_name;
  std::string heightmap_path;
};

struct TerrainRenderState
{
  bool   culling_enabled = true;
  GLenum winding         = GL_CCW;
  GLenum culling_mode    = GL_BACK;
};

class TerrainPiece
{
  glm::vec2           pos_;
  opengl::DrawInfo    di_;
  opengl::TextureInfo ti_;

public:
  //
  // mutable fields
  TerrainPieceConfig config;

  //
  // constructors
  NO_COPY(TerrainPiece);
  MOVE_DEFAULT(TerrainPiece);
  TerrainPiece(TerrainPieceConfig const&, glm::vec2 const&, opengl::DrawInfo&&,
               opengl::TextureInfo const&);

  auto const& position() const { return pos_; }
  auto const& draw_info() const { return di_; }
  auto const& texture_info() const { return ti_; }
};

class TerrainArray
{
  std::vector<TerrainPiece> data_;

public:
  TerrainArray() = default;
  NO_COPY(TerrainArray);
  MOVE_DEFAULT(TerrainArray);

  BEGIN_END_FORWARD_FNS(data_);
  INDEX_OPERATOR_FNS(data_);

  void add(TerrainPiece&&);
  void reserve(size_t);
  auto capacity() const { return data_.capacity(); }
  auto size() const { return data_.size(); }
};

struct TerrainGridConfig
{
  size_t num_rows, num_cols;
  size_t x_length, z_length;

  TerrainGridConfig();
};

class TerrainGrid
{
  TerrainGridConfig config_;
  TerrainArray      terrain_;

public:
  explicit TerrainGrid(TerrainGridConfig const&);

  NO_COPY(TerrainGrid);
  MOVE_DEFAULT(TerrainGrid);
  BEGIN_END_FORWARD_FNS(terrain_);
  INDEX_OPERATOR_FNS(terrain_);

  auto height() const { return config_.num_cols; }
  auto width() const { return config_.num_rows; }
  auto dimensions() const { return stlw::make_array<size_t>(width(), height()); }

  auto count() const { return terrain_.size(); }
  auto size() const { return count(); }
  void add(TerrainPiece&&);

  auto& config() { return config_; }
};

template <typename FN, typename... Args>
void
visit_each(TerrainGrid const& tgrid, FN const& fn, Args&&... args)
{
  auto const [w, h] = tgrid.dimensions();
  FOR(x, w)
  {
    FOR(y, h) { fn(x, y, FORWARD(args)); }
  }
}

struct Terrain
{
  TerrainGrid        grid;
  TerrainRenderState render_state;

  Terrain(TerrainGrid&&);
};

namespace terrain
{

TerrainPiece
generate_piece(stlw::Logger&, glm::vec2 const&, TerrainGridConfig const&, TerrainPieceConfig const&,
               opengl::HeightmapData const&, opengl::ShaderProgram&, opengl::TextureInfo const&);

TerrainGrid
generate_grid(stlw::Logger&, TerrainGridConfig const&, TerrainPieceConfig const&,
              opengl::HeightmapData const&, opengl::ShaderProgram&, opengl::TextureInfo const&);

} // namespace terrain

} // namespace boomhs
