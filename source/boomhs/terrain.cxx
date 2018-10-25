#include <boomhs/mesh.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

#include <cassert>
#include <common/algorithm.hpp>
#include <common/log.hpp>

#include <sstream>

using namespace boomhs;
using namespace opengl;

namespace
{

ObjData
generate_terrain_data(common::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
                      Heightmap const& heightmap)
{
  auto const numv_oneside = tc.num_vertexes_along_one_side;
  auto const num_vertexes = math::squared(numv_oneside);

  ObjData data;
  data.num_vertexes = num_vertexes;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, tgc.dimensions, numv_oneside);
  heightmap::update_vertices_from_heightmap(logger, tc, heightmap, data.vertices);

  {
    GenerateNormalData const gnd{tc.invert_normals, heightmap, numv_oneside};
    data.normals = MeshFactory::generate_normals(logger, gnd);
  }

  data.uvs     = MeshFactory::generate_uvs(logger, tgc.dimensions, numv_oneside, tc.tile_textures);
  data.indices = MeshFactory::generate_indices(logger, numv_oneside);
  return data;
}

TerrainGrid
generate_grid_data(common::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
                   Heightmap const& heightmap, ShaderProgram& sp)
{
  LOG_TRACE("Generating Terrain");
  size_t const rows = tgc.num_rows, cols = tgc.num_cols;
  TerrainGrid  tgrid{tgc};

  FOR(j, rows)
  {
    FOR(i, cols)
    {
      auto const pos = glm::vec2{i, j};
      auto       t   = terrain::generate_piece(logger, pos, tgc, tc, heightmap, sp);
      tgrid.add(MOVE(t));
    }
  }

  LOG_TRACE("Finished Generating Terrain");
  return tgrid;
}

float
barry_centric(glm::vec3 const& p1, glm::vec3 const& p2, glm::vec3 const& p3, glm::vec2 const& pos)
{
  float const det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
  float const l1  = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
  float const l2  = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
  float const l3  = 1.0f - l1 - l2;
  return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainTextureNames
TerrainTextureNames::TerrainTextureNames()
    : heightmap_path("Area0-HM")
    , textures({{"floor0", "grass", "mud", "brick_path", "blendmap"}})
{
}

#define APPEND_COMMA_SEPERATED_LIST(sstr, list, fn)                                                \
  {                                                                                                \
    bool first = true;                                                                             \
    sstr << "{";                                                                                   \
    for (auto const& tn : list) {                                                                  \
      if (!first) {                                                                                \
        sstr << ", ";                                                                              \
      }                                                                                            \
      else {                                                                                       \
        first = false;                                                                             \
      }                                                                                            \
      sstr << fn(tn);                                                                              \
    }                                                                                              \
    sstr << "}";                                                                                   \
  }

std::string
TerrainTextureNames::to_string() const
{
  std::stringstream sstr;
  sstr << "heightmap_path: " << heightmap_path;
  sstr << ", textures: ";

  APPEND_COMMA_SEPERATED_LIST(sstr, textures, [](auto const& t) { return t; });
  return sstr.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainConfig
TerrainConfig::TerrainConfig()
    : num_vertexes_along_one_side(128)
    , height_multiplier(1)
    , invert_normals(false)
    , tile_textures(false)
    , shader_name("terrain")
{
}

std::string
TerrainConfig::to_string() const
{
  return fmt::sprintf(
      "{numv: %lu, height_multiplier %f, invert_normals %i, tile_textures %i, "
      "wrap_mode: %i, uv_max: %f, uv_modifier: %f, shader_name: %s, texture_names: %s}",
      num_vertexes_along_one_side, height_multiplier, invert_normals, tile_textures, wrap_mode,
      uv_max, uv_modifier, shader_name, texture_names.to_string());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain
Terrain::Terrain(TerrainConfig const& tc, glm::vec2 const& pos, DrawInfo&& di, ShaderProgram& sp,
                 Heightmap&& hmap)
    : pos_(pos)
    , di_(MOVE(di))
    , sp_(&sp)
    , config(tc)
    , heightmap(MOVE(hmap))
{
}

std::string
Terrain::to_string() const
{
  return fmt::sprintf("Terrain: {pos: %s, di: %s, sp: %s, terrain config: %s, heightmap: {%s}}",
                      glm::to_string(pos_), di_.to_string(), sp_->to_string(), config.to_string(),
                      heightmap.to_string());
}

std::string&
Terrain::texture_name(size_t const index)
{
  auto& names = this->bound_textures;
  assert(index < names.textures.size());
  return names.textures[index];
}

std::string const&
Terrain::texture_name(size_t const index) const
{
  auto const& names = this->bound_textures;
  assert(index < names.textures.size());
  return names.textures[index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainArray
void
TerrainArray::reserve(size_t const c)
{
  data_.reserve(c);
}

void
TerrainArray::add(Terrain&& t)
{
  data_.emplace_back(MOVE(t));
}

std::string
TerrainArray::to_string() const
{
  std::stringstream sstr;
  sstr << "size: " << data_.size() << " ";
  APPEND_COMMA_SEPERATED_LIST(sstr, *this, [](auto const& t) { return t.to_string(); });
  return sstr.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainGridConfig
TerrainGridConfig::TerrainGridConfig()
    : num_rows(1)
    , num_cols(1)
    , dimensions(20, 20)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainGrid
TerrainGrid::TerrainGrid(TerrainGridConfig const& tgc)
    : config(tgc)
{
  auto const nr = config.num_rows;
  auto const nc = config.num_cols;
  terrain_.reserve(nr * nc);
}

glm::vec2
TerrainGrid::max_worldpositions() const
{
  // The last Terrain in our array will be the further "away" (x,z) coordinates of all the
  // terrains.
  assert(!terrain_.empty());
  auto const& last = terrain_.back();

  auto const dimensions = config.dimensions;
  return (last.position() * dimensions) + dimensions;
}

void
TerrainGrid::add(Terrain&& t)
{
  terrain_.add(MOVE(t));
}

TerrainOutOfBoundsResult
TerrainGrid::out_of_bounds(float const x, float const z) const
{
  auto const max_pos       = this->max_worldpositions();
  bool const x_outofbounds = x >= max_pos.x || x < 0;
  bool const y_outofbounds = z >= max_pos.y || z < 0;
  return TerrainOutOfBoundsResult{x_outofbounds, y_outofbounds};
}

float
TerrainGrid::get_height(common::Logger& logger, float const x, float const z) const
{
  if (out_of_bounds(x, z)) {
    LOG_ERROR_SPRINTF("Player out of bounds");
    return 0.0f;
  }

  auto const& d = config.dimensions;

  auto const get_terrain_under_coords = [&]() -> Terrain const& {
    // Determine which Terrain instance the world coordinates (x, z) fall into.
    size_t const xcoord = x / d.x;
    size_t const zcoord = z / d.y;

    size_t const terrain_index = (num_rows() * zcoord) + xcoord;
    return terrain_[terrain_index];
  };

  auto const& t       = get_terrain_under_coords();
  auto const  t_pos   = t.position();
  float const local_x = x - (t_pos.x * d.x);
  float const local_z = z - (t_pos.y * d.y);
  assert(!out_of_bounds(local_x, local_z));

  float const num_vertexes_minus1 = t.config.num_vertexes_along_one_side - 1;

  assert(d.x == d.y);
  float const grid_squaresize = d.x / num_vertexes_minus1;

  size_t const grid_x = glm::floor(local_x / grid_squaresize);
  size_t const grid_z = glm::floor(local_z / grid_squaresize);

  float const x_coord = ::fmodf(local_x, grid_squaresize) / grid_squaresize;
  float const z_coord = ::fmodf(local_z, grid_squaresize) / grid_squaresize;

  glm::vec3       p1, p2, p3;
  glm::vec2 const p4 = glm::vec2{x_coord, z_coord};

  auto const& hmap = t.heightmap;
  if (x_coord <= (1.0f - z_coord)) {
    p1 = glm::vec3{0, hmap.data(grid_x, grid_z), 0};
    p2 = glm::vec3{1, hmap.data(grid_x + 1, grid_z), 0};
    p3 = glm::vec3{0, hmap.data(grid_x, grid_z + 1), 1};
  }
  else {
    p1 = glm::vec3{1, hmap.data(grid_x + 1, grid_z), 0};
    p2 = glm::vec3{1, hmap.data(grid_x + 1, grid_z + 1), 1};
    p3 = glm::vec3{0, hmap.data(grid_x, grid_z + 1), 1};
  }
  float const theight         = barry_centric(p1, p2, p3, p4);
  float const bitmap_adjusted = theight / 255.0f;
  return bitmap_adjusted * t.config.height_multiplier;
}

std::string
TerrainGrid::to_string() const
{
  std::stringstream sstr;
  sstr << fmt::sprintf("{culling_enabled: %i, winding: %i, culling_mode: %i, TerrainArray: %s}",
                       culling_enabled, winding, culling_mode, terrain_.to_string());
  // APPEND_COMMA_SEPERATED_LIST(sstr, *this, [](auto const& t) { return t.to_string(); });
  sstr << " ";
  return sstr.str();
}

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate_piece(common::Logger& logger, glm::vec2 const& pos, TerrainGridConfig const& tgc,
               TerrainConfig const& tc, Heightmap const& heightmap, ShaderProgram& sp)
{
  auto const data = generate_terrain_data(logger, tgc, tc, heightmap);
  LOG_TRACE_SPRINTF("Generated terrain piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, sp.va(), buffer);

  // These uniforms only need to be set once.
  sp.while_bound(logger, [&]() {
    sp.set_uniform(logger, "u_bgsampler", 0);
    sp.set_uniform(logger, "u_rsampler", 1);
    sp.set_uniform(logger, "u_gsampler", 2);
    sp.set_uniform(logger, "u_bsampler", 3);
    sp.set_uniform(logger, "u_blendsampler", 4);
  });

  return Terrain{tc, pos, MOVE(di), sp, heightmap.clone()};
}

TerrainGrid
generate_grid(common::Logger& logger, TerrainConfig const& tc, Heightmap const& heightmap,
              ShaderProgram& sp, TerrainGrid const& prevgrid)
{
  auto tgrid = generate_grid_data(logger, prevgrid.config, tc, heightmap, sp);

  // If the previous grid has enough rows/columns for how far along we are generating a new grid,
  // then copy the previous terrain's config to the new terrain.
  size_t const rows = tgrid.config.num_rows, cols = tgrid.config.num_cols;
  FOR(j, rows)
  {
    FOR(i, cols)
    {
      bool const prevgrid_grid_enough_elements = (rows * cols) < prevgrid.size();
      bool const within_rows_and_columns       = j < rows && i < cols;
      if (prevgrid_grid_enough_elements && within_rows_and_columns) {
        auto const index    = (j * rows) + i;
        tgrid[index].config = prevgrid[index].config;
      }
    }
  }
  return tgrid;
}

TerrainGrid
generate_grid(common::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
              Heightmap const& heightmap, ShaderProgram& sp)
{
  return generate_grid_data(logger, tgc, tc, heightmap, sp);
}

} // namespace boomhs::terrain
