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

  bool constexpr TILE = true;
  data.uvs            = MeshFactory::generate_uvs(logger, tgc.dimensions, numv_oneside, TILE);
  data.indices        = MeshFactory::generate_indices(logger, numv_oneside);
  return data;
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainConfig
TerrainConfig::TerrainConfig()
    : num_vertexes_along_one_side(128)
    , height_multiplier(1)
    , invert_normals(false)
    , shader_name("terrain")
    , texture_name("Floor0")
    , heightmap_path("Area0-HM")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain
Terrain::Terrain(TerrainConfig const& tc, glm::vec2 const& pos, DrawInfo&& di, ShaderProgram& sp,
                 TextureInfo& ti, Heightmap&& hmap)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(&ti)
    , sp_(&sp)
    , config(tc)
    , heightmap(MOVE(hmap))
{
  pos_ = pos;
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

void
TerrainGrid::add(Terrain&& t)
{
  terrain_.add(MOVE(t));
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

float
TerrainGrid::get_height(stlw::Logger& logger, float const x, float const z) const
{
  auto const& d = dimensions();

  auto const get_terrain_under_coords = [&]() -> Terrain const& {
    // Determine which Terrain instance the world coordinates (x, z) fall into.
    size_t const xcoord = x / d.x;
    size_t const zcoord = z / d.y;

    size_t const terrain_index = (num_rows() * zcoord) + xcoord;
    return terrain_[terrain_index];
  };

  auto const& t       = get_terrain_under_coords();
  float const local_x = x - t.position().x;
  float const local_z = z - t.position().y;

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

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate_piece(stlw::Logger& logger, glm::vec2 const& pos, TerrainGridConfig const& tgc,
               TerrainConfig const& tc, Heightmap const& heightmap, ShaderProgram& sp,
               TextureInfo& ti)
{
  auto const data = generate_terrain_data(logger, tgc, tc, heightmap);
  LOG_TRACE_SPRINTF("Generated terrain piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, sp.va(), buffer);

  // These uniforms only need to be set once.
  sp.while_bound(logger, [&]() { sp.set_uniform_int1(logger, "u_sampler", 0); });

  return Terrain{tc, pos, MOVE(di), sp, ti, heightmap.clone()};
}

TerrainGrid
generate_grid(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
              Heightmap const& heightmap, ShaderProgram& sp, TextureInfo& ti)
{
  LOG_TRACE("Generating Terrain");
  size_t const rows = tgc.num_rows, cols = tgc.num_cols;
  TerrainGrid  tgrid{tgc};

  FOR(j, rows)
  {
    FOR(i, cols)
    {
      auto const pos = glm::vec2{i, j};
      auto       t   = generate_piece(logger, pos, tgc, tc, heightmap, sp, ti);

      auto const index = (j * rows) + i;

      // TODO: use index here?
      tgrid.add(MOVE(t));
    }
  }

  LOG_TRACE("Finished Generating Terrain");
  return tgrid;
}

} // namespace boomhs::terrain
