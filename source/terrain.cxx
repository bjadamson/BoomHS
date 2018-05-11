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
generate_terrain_data(stlw::Logger& logger, TerrainGridConfig const& tgc,
                      TerrainPieceConfig const& tc, HeightmapData const& heightmap_data)
{
  auto const numv_oneside = tc.num_vertexes_along_one_side;
  auto const num_vertexes = stlw::math::squared(numv_oneside);

  ObjData data;
  data.num_vertexes = num_vertexes;

  data.vertices = MeshFactory::generate_rectangle_mesh(logger, tgc.dimensions, numv_oneside);
  heightmap::update_vertices_from_heightmap(logger, tc, heightmap_data, data.vertices);

  {
    glm::vec2 const          dimensions{1};
    GenerateNormalData const gnd{tc.invert_normals, heightmap_data, numv_oneside};
    data.normals = MeshFactory::generate_normals(gnd);
  }

  data.uvs     = MeshFactory::generate_uvs(tgc.dimensions, numv_oneside);
  data.indices = MeshFactory::generate_indices(numv_oneside);
  return data;
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainPieceConfig
TerrainPieceConfig::TerrainPieceConfig()
    : num_vertexes_along_one_side(128)
    , height_multiplier(1)
    , invert_normals(false)
    , shader_name("terrain")
    , texture_name("Floor0")
    , heightmap_path("Area0-HM")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainPiece
TerrainPiece::TerrainPiece(TerrainPieceConfig const& tc, glm::vec2 const& pos, DrawInfo&& di,
                           ShaderProgram& sp, TextureInfo const& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(ti)
    , sp_(sp)
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
TerrainArray::add(TerrainPiece&& t)
{
  data_.emplace_back(MOVE(t));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainGridConfig
TerrainGridConfig::TerrainGridConfig()
    : num_rows(1)
    , num_cols(1)
    , dimensions(8, 8)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainGrid
TerrainGrid::TerrainGrid(TerrainGridConfig const& tgc)
    : config_(tgc)
{
  auto const nr = config_.num_rows;
  auto const nc = config_.num_cols;
  terrain_.reserve(nr * nc);
}

void
TerrainGrid::add(TerrainPiece&& t)
{
  terrain_.add(MOVE(t));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain
Terrain::Terrain(TerrainGrid&& tgrid)
    : grid(MOVE(tgrid))
{
}

} // namespace boomhs

namespace boomhs::terrain
{

TerrainPiece
generate_piece(stlw::Logger& logger, glm::vec2 const& pos, TerrainGridConfig const& tgc,
               TerrainPieceConfig const& tc, HeightmapData const& heightmap_data, ShaderProgram& sp,
               TextureInfo const& ti)
{
  auto const data = generate_terrain_data(logger, tgc, tc, heightmap_data);
  LOG_TRACE_SPRINTF("Generated terrain piece: %s", data.to_string());

  BufferFlags const flags{true, true, false, true};
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto              di     = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer);

  // These uniforms only need to be set once.
  sp.while_bound(logger, [&]() { sp.set_uniform_int1(logger, "u_sampler", 0); });

  return TerrainPiece{tc, pos, MOVE(di), sp, ti};
}

TerrainGrid
generate_grid(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainPieceConfig const& tc,
              HeightmapData const& heightmap_data, ShaderProgram& sp, TextureInfo const& ti)
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
