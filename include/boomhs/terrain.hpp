#pragma once
#include <extlibs/glm.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/heightmap.hpp>

#include <ostream>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace boomhs
{

struct TerrainConfiguration
{
  TerrainConfiguration();

  size_t num_vertexes;
  size_t x_length, z_length;
  size_t num_rows, num_cols;
  size_t height_multiplier;
  bool   invert_normals;

  GLenum winding   = GL_CCW;
  GLint  wrap_mode = GL_REPEAT;

  bool   culling_enabled = true;
  GLenum culling_mode    = GL_BACK;
  float  uv_max          = 1.0f;
  float  uv_modifier     = 1.0f;

  std::string shader_name;
  std::string texture_name;
  std::string heightmap_path;
};

class Terrain
{
  glm::vec2           pos_;
  opengl::DrawInfo    di_;
  opengl::TextureInfo ti_;

public:
  //
  // mutable fields
  TerrainConfiguration config;

  //
  // constructors
  NO_COPY(Terrain);
  MOVE_DEFAULT(Terrain);
  Terrain(TerrainConfiguration const&, glm::vec2 const&, opengl::DrawInfo&&,
          opengl::TextureInfo const&);

  auto const& position() const { return pos_; }
  auto const& draw_info() const { return di_; }
  auto const& texture_info() const { return ti_; }
};

class TerrainArray
{
  std::vector<Terrain> data_;

public:
  TerrainArray() = default;
  NO_COPY(TerrainArray);
  MOVE_DEFAULT(TerrainArray);

  BEGIN_END_FORWARD_FNS(data_);
  INDEX_OPERATOR_FNS(data_);

  void add(Terrain&&);
  void reserve(size_t);
  auto capacity() const { return data_.capacity(); }
  auto size() const { return data_.size(); }
};

class TerrainGrid
{
  size_t       num_rows_, num_cols_;
  TerrainArray terrain_;

public:
  explicit TerrainGrid(size_t, size_t);
  TerrainGrid();

  NO_COPY(TerrainGrid);
  MOVE_DEFAULT(TerrainGrid);
  BEGIN_END_FORWARD_FNS(terrain_);
  INDEX_OPERATOR_FNS(terrain_);

  auto height() const { return num_cols_; }
  auto width() const { return num_rows_; }

  auto count() const { return terrain_.size(); }
  auto size() const { return count(); }
  void add(Terrain&&);
};

namespace terrain
{

TerrainGrid
generate(stlw::Logger&, TerrainConfiguration const&, opengl::HeightmapData const&,
         opengl::ShaderProgram&, opengl::TextureInfo const&);

} // namespace terrain

} // namespace boomhs
