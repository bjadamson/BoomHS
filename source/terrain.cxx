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
                      HeightmapData const& heightmap_data)
{
  auto const numv_oneside = tc.num_vertexes_along_one_side;
  auto const num_vertexes = stlw::math::squared(numv_oneside);

  ObjData data;
  data.num_vertexes = num_vertexes;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, tgc.dimensions, numv_oneside);
  heightmap::update_vertices_from_heightmap(logger, tc, heightmap_data, data.vertices);

  {
    GenerateNormalData const gnd{tc.invert_normals, heightmap_data, numv_oneside};
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
                 TextureInfo& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(&ti)
    , sp_(&sp)
    , config(tc)
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

} // namespace boomhs

namespace boomhs::terrain
{

Terrain
generate_piece(stlw::Logger& logger, glm::vec2 const& pos, TerrainGridConfig const& tgc,
               TerrainConfig const& tc, HeightmapData const& heightmap_data, ShaderProgram& sp,
               TextureInfo& ti)
{
  auto const data = generate_terrain_data(logger, tgc, tc, heightmap_data);
  LOG_TRACE_SPRINTF("Generated terrain piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, sp.va(), buffer);

  // These uniforms only need to be set once.
  sp.while_bound(logger, [&]() { sp.set_uniform_int1(logger, "u_sampler", 0); });

  return Terrain{tc, pos, MOVE(di), sp, ti};
}

TerrainGrid
generate_grid(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainConfig const& tc,
              HeightmapData const& heightmap_data, ShaderProgram& sp, TextureInfo& ti)
{
  LOG_TRACE("Generating Terrain");
  size_t const rows = tgc.num_rows, cols = tgc.num_cols;
  TerrainGrid  tgrid{tgc};

  FOR(i, rows)
  {
    FOR(j, cols)
    {
      auto const pos = glm::vec2{i, j};
      auto       t   = generate_piece(logger, pos, tgc, tc, heightmap_data, sp, ti);

      auto const index = (j * rows) + i;

      // TODO: use index here?
      tgrid.add(MOVE(t));
    }
  }

  LOG_TRACE("Finished Generating Terrain");
  return tgrid;
}

} // namespace boomhs::terrain
