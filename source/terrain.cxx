#include <boomhs/mesh.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
#include <opengl/heightmap.hpp>
#include <opengl/shader.hpp>

#include <cassert>
#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

#include <sstream>

using namespace boomhs;
using namespace opengl;

namespace
{

ObjData
generate_terrain_data(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
                      Heightmap const& heightmap)
{
  auto const numv_oneside = tc.num_vertexes_along_one_side;
  auto const num_vertexes = stlw::math::squared(numv_oneside);

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

#define APPEND_COMMA_SEPERATED_LIST(sstr, list, fn) \
  { \
    bool first = true; \
    sstr << "{"; \
    for (auto const& tn : list) { \
      if (!first) { \
        sstr << ", "; \
      } \
      else { \
        first = false; \
      }\
      sstr << fn(tn); \
    } \
    sstr << "}"; \
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
  return fmt::sprintf("\n  TerrainConfig: {numv: %lu, height_multiplier %f, invert_normals %i, tile_textures %i, "
      "wrap_mode: %i, uv_max: %f, uv_modifier: %f, shader_name: %s, texture_names: %s}",
      num_vertexes_along_one_side,
      height_multiplier,
      invert_normals,
      tile_textures,
      wrap_mode,
      uv_max,
      uv_modifier,
      shader_name,
      texture_names.to_string());
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
  pos_ = pos;
}

Terrain::Terrain(Terrain&& other)
    : pos_(other.pos_)
    , di_(MOVE(other.di_))
    , sp_(other.sp_)
    , config(MOVE(other.config))
    , heightmap(MOVE(other.heightmap))
    , debug_check(MOVE(other.debug_check))
    , bound_textures(MOVE(other.bound_textures))
{
}

Terrain&
Terrain::operator=(Terrain&& other)
{
  this->pos_ = other.pos_;
  this->di_ = MOVE(other.di_);
  this->sp_ = MOVE(other.sp_);
  this->config = MOVE(other.config);
  this->heightmap = MOVE(other.heightmap);
  this->debug_check = MOVE(other.debug_check);
  this->bound_textures = MOVE(other.bound_textures);
  return *this;
}

void
Terrain::bind_impl(stlw::Logger& logger, opengl::TextureTable& ttable)
{
  auto const bind = [&](size_t const tunit) {
    glActiveTexture(GL_TEXTURE0 + tunit);
    auto& tinfo = *ttable.find(texture_name(tunit));
    bind::global_bind(logger, tinfo);
  };

  bound_textures.names = config.texture_names;
  FOR(i, bound_textures.names.textures.size()) { bind(i); }
}

void
Terrain::unbind_impl(stlw::Logger& logger, opengl::TextureTable& ttable)
{
  auto const unbind = [&](size_t const tunit) {
    auto& tinfo = *ttable.find(texture_name(tunit));
    bind::global_unbind(logger, tinfo);
  };

  FOR(i, bound_textures.names.textures.size()) { unbind(i); }
  glActiveTexture(GL_TEXTURE0);
}

std::string
Terrain::to_string() const
{
  return fmt::sprintf("\n  Terrain: {pos: %s, di: %s, sp: %s, config: %s, heightmap: %s}",
      glm::to_string(pos_),
      di_.to_string(),
      sp_->to_string(),
      config.to_string(),
      heightmap.to_string());
}

std::string&
Terrain::texture_name(size_t const index)
{
  auto& names = bound_textures.names;
  assert(index < names.textures.size());
  return names.textures[index];
}

std::string const&
Terrain::texture_name(size_t const index) const
{
  auto const& names = bound_textures.names;
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
  sstr << "TerrainArray: \n";
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

  auto const tile_dimensions = config.dimensions;
  return (last.position() * tile_dimensions) + tile_dimensions;
}

void
TerrainGrid::add(Terrain&& t)
{
  terrain_.add(MOVE(t));
}

float
TerrainGrid::get_height(stlw::Logger& logger, float const x, float const z) const
{
  auto const& d = config.dimensions;

  auto const get_terrain_under_coords = [&]() -> Terrain const& {
    // Determine which Terrain instance the world coordinates (x, z) fall into.
    size_t const xcoord = x / d.x;
    size_t const zcoord = z / d.y;

    size_t const terrain_index = (num_rows() * zcoord) + xcoord;
    return terrain_[terrain_index];
  };

  auto const& t       = get_terrain_under_coords();
  float const local_x = x - (t.position().x * d.x);
  float const local_z = z - (t.position().y * d.y);

  float const num_vertexes_minus1 = t.config.num_vertexes_along_one_side - 1;

  assert(d.x == d.y);
  float const grid_squaresize = d.x / num_vertexes_minus1;

  size_t const grid_x = glm::floor(local_x / grid_squaresize);
  size_t const grid_z = glm::floor(local_z / grid_squaresize);

  if (grid_x >= num_vertexes_minus1 || grid_z >= num_vertexes_minus1 || grid_x < 0 || grid_z < 0) {
    LOG_ERROR_SPRINTF("Player out of bounds");
    return 0.0f;
  }

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
  float const theight = barry_centric(p1, p2, p3, p4);
  return theight / 255.0f;
}

std::string
TerrainGrid::to_string() const
{
  std::stringstream sstr;
  sstr << "TerrainGrid: ";
  sstr << fmt::sprintf("culling_enabled: %i, winding: %i, culling_mode: %i\n",
      culling_enabled, winding, culling_mode);
  APPEND_COMMA_SEPERATED_LIST(sstr, *this, [](auto const& t) { return t.to_string(); });
  return sstr.str();
}

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate_piece(stlw::Logger& logger, glm::vec2 const& pos, TerrainGridConfig const& tgc,
               TerrainConfig const& tc, Heightmap const& heightmap, ShaderProgram& sp)
{
  auto const data = generate_terrain_data(logger, tgc, tc, heightmap);
  LOG_TRACE_SPRINTF("Generated terrain piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, sp.va(), buffer);

  // These uniforms only need to be set once.
  sp.while_bound(logger, [&]() {
    sp.set_uniform_int1(logger, "u_bgsampler", 0);
    sp.set_uniform_int1(logger, "u_rsampler", 1);
    sp.set_uniform_int1(logger, "u_gsampler", 2);
    sp.set_uniform_int1(logger, "u_bsampler", 3);
    sp.set_uniform_int1(logger, "u_blendsampler", 4);
  });

  return Terrain{tc, pos, MOVE(di), sp, heightmap.clone()};
}

TerrainGrid
generate_grid(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
              Heightmap const& heightmap, ShaderProgram& sp, TerrainGrid* prevgrid)
{
  LOG_TRACE("Generating Terrain");
  size_t const rows = tgc.num_rows, cols = tgc.num_cols;
  TerrainGrid  tgrid{tgc};

  FOR(j, rows)
  {
    FOR(i, cols)
    {
      auto const pos = glm::vec2{j, i};
      auto       t   = generate_piece(logger, pos, tgc, tc, heightmap, sp);

      // If the user provided a previous grid, and the previous grid has enough rows/columns for
      // how far along we are generating a new grid, then copy the previous terrain's config to the
      // new terrain.
      if (prevgrid && prevgrid->num_rows() >= j && prevgrid->num_cols() >= i) {
        //auto const index = (j * rows) + i;
        //t.config = (*prevgrid)[index].config;
      }

      tgrid.add(MOVE(t));
    }
  }

  LOG_TRACE("Finished Generating Terrain");
  return tgrid;
}

TerrainGrid
generate_grid(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
              Heightmap const& heightmap, ShaderProgram& sp)
{
  return generate_grid(logger, tgc, tc, heightmap, sp, nullptr);
}

} // namespace boomhs::terrain
