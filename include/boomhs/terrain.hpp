#pragma once
#include <boomhs/heightmap.hpp>
#include <opengl/draw_info.hpp>

#include <common/algorithm.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>

#include <array>
#include <functional>
#include <vector>

namespace opengl
{
class ShaderProgram;
class TextureTable;
} // namespace opengl

namespace boomhs
{

struct TerrainTextureNames
{
  std::string              heightmap_path;
  std::vector<std::string> textures;

  TerrainTextureNames();
  std::string to_string() const;
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

  std::string to_string() const;
};

class Terrain
{
  glm::vec2              pos_;
  opengl::DrawInfo       di_;
  opengl::ShaderProgram* sp_;

public:
  NO_COPY(Terrain);
  MOVE_DEFAULT(Terrain);

  Terrain(TerrainConfig const&, glm::vec2 const&, opengl::DrawInfo&&, opengl::ShaderProgram&,
          Heightmap&&);

  // public members
  TerrainConfig       config;
  Heightmap           heightmap;
  TerrainTextureNames bound_textures;

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

  std::string to_string() const;
};

struct TerrainGridConfig
{
  size_t    num_rows, num_cols;
  glm::vec2 dimensions;

  TerrainGridConfig();
};

// The result of checking whether a position is outside of the terrain grid.
//
// It is implicitely convertible to a bool for easy checking, or you can check the individual
// components if you need.
struct TerrainOutOfBoundsResult
{
  bool const x;
  bool const z;
  explicit TerrainOutOfBoundsResult(bool const xp, bool const zp)
      : x(xp)
      , z(zp)
  {
  }

  bool     is_out() const { return x || z; }
  explicit operator bool() const { return is_out(); }
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
  auto      rows_and_columns() const { return common::make_array<size_t>(num_rows(), num_cols()); }

  void add(Terrain&&);

  auto count() const { return terrain_.size(); }
  auto size() const { return count(); }

  float                    get_height(common::Logger&, float, float) const;
  TerrainOutOfBoundsResult out_of_bounds(float, float) const;
  std::string              to_string() const;
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
generate_piece(common::Logger&, glm::vec2 const&, TerrainGridConfig const&, TerrainConfig const&,
               Heightmap const&, opengl::ShaderProgram&);
TerrainGrid
generate_grid(common::Logger&, TerrainConfig const&, Heightmap const&, opengl::ShaderProgram&,
              TerrainGrid const&);

TerrainGrid
generate_grid(common::Logger&, TerrainGridConfig const&, TerrainConfig const&, Heightmap const&,
              opengl::ShaderProgram&);

} // namespace boomhs::terrain
