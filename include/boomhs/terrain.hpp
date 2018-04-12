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

class TerrainArray
{
  std::vector<Terrain> data_;
  size_t               num_inserted_ = 0;

public:
  decltype(auto) begin() { return data_.begin(); }
  decltype(auto) end() { return std::next(begin(), num_inserted_); }

  decltype(auto) begin() const { return data_.begin(); }
  decltype(auto) end() const { return std::next(begin(), num_inserted_); }

  decltype(auto) cbegin() const { return data_.cbegin(); }
  decltype(auto) cend() const { return std::next(begin(), num_inserted_); }

  void set(size_t, Terrain&&);
  void reserve(size_t);
  auto capacity() const { return data_.capacity(); }
  auto size() const { return num_inserted_; }
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

  auto height() const { return num_cols_; }
  auto width() const { return num_rows_; }

  auto count() const { return terrain_.size(); }
  void set(size_t, Terrain&&);
};

struct TerrainConfiguration
{
  size_t num_vertexes;
  size_t x_length, z_length;
  size_t num_rows, num_cols;

  std::string shader_name;
  std::string texture_name;
  std::string heightmap_path;

  TerrainConfiguration();
};

namespace terrain
{

TerrainGrid
generate(stlw::Logger&, TerrainConfiguration const&, opengl::HeightmapData const&,
         opengl::ShaderProgram&, opengl::TextureInfo const&);

} // namespace terrain

} // namespace boomhs
