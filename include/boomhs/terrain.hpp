#pragma once
#include <opengl/bind.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/heightmap.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <array>
#include <functional>
#include <vector>

namespace opengl
{
class ShaderProgram;
class TextureTable;
} // namespace opengl

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class EntityRegistry;
struct RenderState;

struct TerrainTextureNames
{
  std::string heightmap_path;

  std::vector<std::string> textures;

  TerrainTextureNames();
};

struct TerrainConfig
{
  TerrainConfig();

  size_t num_vertexes_along_one_side;
  float  height_multiplier;
  bool   invert_normals;
  bool   tile_textures;

  GLint wrap_mode   = GL_MIRRORED_REPEAT;
  float uv_max      = 1.0f;
  float uv_modifier = 1.0f;

  std::string         shader_name;
  TerrainTextureNames texture_names;
};

class Terrain
{
  glm::vec2              pos_;
  opengl::DrawInfo       di_;
  opengl::ShaderProgram* sp_;

public:
  //
  // mutable fields
  TerrainConfig     config;
  opengl::Heightmap heightmap;

  //
  // constructors
  NO_COPY(Terrain);
  MOVE_DEFAULT(Terrain);
  Terrain(TerrainConfig const&, glm::vec2 const&, opengl::DrawInfo&&, opengl::ShaderProgram&,
          opengl::Heightmap&&);

  // public members
  opengl::DebugBoundCheck debug_check;

  void bind_impl(stlw::Logger&, opengl::TextureTable&);
  void unbind_impl(stlw::Logger&, opengl::TextureTable&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  auto&       draw_info() { return di_; }
  auto const& position() const { return pos_; }

  std::string&       texture_name(size_t);
  std::string const& texture_name(size_t) const;

  auto&       shader() { return *sp_; }
  std::string to_string() const;
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

  bool        empty() const { return data_.empty(); }
  auto const& back() const { return data_.back(); }
};

struct TerrainGridConfig
{
  size_t    num_rows, num_cols;
  glm::vec2 dimensions;

  TerrainGridConfig();
};

class TerrainGrid
{
  TerrainArray terrain_;

public:
  explicit TerrainGrid(TerrainGridConfig const&);

  NO_COPY(TerrainGrid);
  MOVE_DEFAULT(TerrainGrid);
  BEGIN_END_FORWARD_FNS(terrain_);
  INDEX_OPERATOR_FNS(terrain_);

  // fields
  TerrainGridConfig config;
  bool              culling_enabled = true;
  GLenum            winding         = GL_CCW;
  GLenum            culling_mode    = GL_BACK;

  // methods
  auto num_cols() const { return config.num_cols; }
  auto num_rows() const { return config.num_rows; }

  glm::vec2 max_worldpositions() const;
  auto      rows_and_columns() const { return stlw::make_array<size_t>(num_rows(), num_cols()); }

  auto count() const { return terrain_.size(); }
  auto size() const { return count(); }
  void add(Terrain&&);

  float get_height(stlw::Logger&, float, float) const;
};

template <typename FN, typename... Args>
void
visit_each(TerrainGrid const& tgrid, FN const& fn, Args&&... args)
{
  auto const [row, col] = tgrid.rows_and_columns();
  FOR(x, row)
  {
    FOR(y, col) { fn(x, y, FORWARD(args)); }
  }
}

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate_piece(stlw::Logger&, glm::vec2 const&, TerrainGridConfig const&, TerrainConfig const&,
               opengl::Heightmap const&, opengl::ShaderProgram&);

TerrainGrid
generate_grid(stlw::Logger&, TerrainGridConfig const&, TerrainConfig const&,
              opengl::Heightmap const&, opengl::ShaderProgram&);

} // namespace boomhs::terrain
