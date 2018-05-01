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

// num_components => number elements per vertex
//
// ie: positions would have "3" for (x, y, z)
//
//     normals              "3" for (xn, yn, zn)
//     uvs                  "2" for (u, v)
//
// etc...
size_t
calculate_number_vertices(int const num_components, TerrainPieceConfig const& tc)
{
  return num_components * (tc.num_vertexes * tc.num_vertexes);
}

float
x_ratio(float const x, TerrainGridConfig const& tgc, TerrainPieceConfig const& tc)
{
  return (x / (tc.num_vertexes - 1)) * tgc.x_length;
}

float
z_ratio(float const z, TerrainGridConfig const& tgc, TerrainPieceConfig const& tc)
{
  return (z / (tc.num_vertexes - 1)) * tgc.z_length;
}

ObjData::vertices_t
generate_vertices(stlw::Logger& logger, TerrainGridConfig const& tgc, TerrainPieceConfig const& tc,
                  HeightmapData const& heightmap_data)
{
  int constexpr NUM_COMPONENTS = 3; // x, y, z
  auto const          x_length = tc.num_vertexes, z_length = tc.num_vertexes;
  auto const          num_vertexes = calculate_number_vertices(NUM_COMPONENTS, tc);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertexes);

  size_t offset = 0;
  assert(offset < buffer.size());
  FOR(z, z_length)
  {
    FOR(x, x_length)
    {
      assert(offset < static_cast<size_t>(num_vertexes));

      float const x_position = 0.0f + (x_ratio(x, tgc, tc));
      float const z_position = 0.0f + (z_ratio(z, tgc, tc));

      assert(offset < buffer.size());
      buffer[offset++] = x_position;
      assert(offset < buffer.size());

      uint8_t const height            = heightmap_data.data()[(x_length * z) + x];
      float const   height_normalized = height / 255.0f;

      LOG_TRACE_SPRINTF("TERRAIN HEIGHT: %f (raw: %u)", height_normalized, height);
      assert(height >= 0.0f);

      buffer[offset++] = height_normalized * tc.height_multiplier;

      assert(offset < buffer.size());
      buffer[offset++] = z_position;
    }
  }

  assert(offset == buffer.size());
  assert(offset == static_cast<size_t>(num_vertexes));
  return buffer;
}

ObjData::vertices_t
generate_uvs(TerrainGridConfig const& tgc, TerrainPieceConfig const& tc)
{
  auto const          num_vertexes = calculate_number_vertices(2, tc);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertexes);

  size_t counter = 0;
  FOR(x, tc.num_vertexes)
  {
    FOR(z, tc.num_vertexes)
    {
      assert(counter < num_vertexes);

      float const u = 0.0f + x_ratio(x, tgc, tc);
      float const v = 0.0f + z_ratio(z, tgc, tc);

      buffer[counter++] = u;
      buffer[counter++] = v;
    }
  }
  assert(counter == num_vertexes);
  return buffer;
}

ObjData::indices_t
generate_indices(TerrainPieceConfig const& tc)
{
  auto const x_length = tc.num_vertexes, z_length = tc.num_vertexes;

  auto const strips_required          = z_length - 1;
  auto const degen_triangles_required = 2 * (strips_required - 1);
  auto const vertices_perstrip        = 2 * x_length;

  size_t const num_indices = (vertices_perstrip * strips_required) + degen_triangles_required;

  ObjData::indices_t buffer;
  buffer.resize(num_indices);

  size_t offset = 0;
  FOR(z, z_length - 1)
  {
    if (z > 0) {
      // Degenerate begin: repeat first vertex
      buffer[offset++] = z * z_length;
    }

    FOR(x, x_length)
    {
      // One part of the strip
      buffer[offset++] = (z * z_length) + x;
      buffer[offset++] = ((z + 1) * z_length) + x;
    }

    if (z < (z_length - 2)) {
      // Degenerate end: repeat last vertex
      buffer[offset++] = ((z + 1) * z_length) + (x_length - 1);
    }
  }

  return buffer;
}

// Algorithm modified from:
// https://www.youtube.com/watch?v=yNYwZMmgTJk&list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP&index=14
ObjData
generate_terrain_data(stlw::Logger& logger, BufferFlags const& flags,
                      TerrainGridConfig const& tgc, TerrainPieceConfig const& tc,
                      HeightmapData const& heightmap_data)
{
  auto const count = tc.num_vertexes * tc.num_vertexes;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = generate_vertices(logger, tgc, tc, heightmap_data);
  data.normals  = heightmap::generate_normals(tc.num_vertexes, tc.num_vertexes, tc.invert_normals,
                                             heightmap_data);

  data.uvs     = generate_uvs(tgc, tc);
  data.indices = generate_indices(tc);
  return data;
}



} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainPieceConfig
TerrainPieceConfig::TerrainPieceConfig()
    : num_vertexes(64)
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
                 TextureInfo const& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(ti)
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
    , x_length(1)
    , z_length(1)
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
Terrain::Terrain(TerrainGrid &&tgrid)
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
  BufferFlags const flags{true, true, false, true};
  auto const        data = generate_terrain_data(logger, flags, tgc, tc, heightmap_data);
  LOG_DEBUG_SPRINTF("Generated terrain data: %s", data.to_string());

  auto const buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto       di     = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, ti);

  LOG_TRACE("Finished Generating Terrain");
  return TerrainPiece{tc, pos, MOVE(di), ti};
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
