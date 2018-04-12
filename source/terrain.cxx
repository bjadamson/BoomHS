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
calculate_number_vertices(int const num_components, TerrainConfiguration const& tc)
{
  return num_components * (tc.num_vertexes * tc.num_vertexes);
}

float
x_ratio(float const x, TerrainConfiguration const& tc)
{
  return (x / (tc.num_vertexes - 1)) * tc.x_length;
}

float
z_ratio(float const z, TerrainConfiguration const& tc)
{
  return (1.0f - (z / (tc.num_vertexes - 1))) * tc.z_length;
}

ObjData::vertices_t
generate_vertices(stlw::Logger& logger, TerrainConfiguration const& tc,
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

      float const x_position = 0.0f + (x_ratio(x, tc));
      float const z_position = 0.0f + (z_ratio(z, tc));

      assert(offset < buffer.size());
      buffer[offset++] = x_position;

      assert(offset < buffer.size());

      uint8_t const height   = heightmap_data.data()[(x_length * z) + x];
      float const   height_f = height / 255.0f;
      LOG_TRACE_SPRINTF("TERRAIN HEIGHT: %f (raw: %u)", height_f, height);
      buffer[offset++] = height_f;

      assert(offset < buffer.size());
      buffer[offset++] = z_position;
    }
  }

  assert(offset == buffer.size());
  assert(offset == static_cast<size_t>(num_vertexes));
  return buffer;
}

ObjData::vertices_t
generate_uvs(TerrainConfiguration const& tc)
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

      float const u = 0.0f + x_ratio(x, tc);
      float const v = 0.0f + z_ratio(z, tc);

      buffer[counter++] = u;
      buffer[counter++] = v;
    }
  }
  assert(counter == num_vertexes);
  return buffer;
}

ObjData::indices_t
generate_indices(TerrainConfiguration const& tc)
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
                      TerrainConfiguration const& tc, HeightmapData const& heightmap_data)
{
  auto const count = tc.num_vertexes * tc.num_vertexes;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = generate_vertices(logger, tc, heightmap_data);
  data.normals  = heightmap::generate_normals(tc.num_vertexes, tc.num_vertexes, heightmap_data);
  data.uvs      = generate_uvs(tc);
  data.indices  = generate_indices(tc);
  return data;
}

auto
generate_terrain_tile(stlw::Logger& logger, glm::vec2 const& pos, TerrainConfiguration const& tc,
                      HeightmapData const& heightmap_data, ShaderProgram& sp, TextureInfo const& ti)
{
  BufferFlags const flags{true, true, false, true};
  auto const        data = generate_terrain_data(logger, flags, tc, heightmap_data);
  LOG_DEBUG_SPRINTF("Generated terrain data: %s", data.to_string());

  auto const buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto       di     = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, ti);

  LOG_TRACE("Finished Generating Terrain");
  return Terrain{pos, MOVE(di), ti};
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainConfiguration
TerrainConfiguration::TerrainConfiguration()
    : num_vertexes(256)
    , x_length(1)
    , z_length(1)
    , num_rows(1)
    , num_cols(1)
    , shader_name("terrain")
    , texture_name("TerrainFloor")
    , heightmap_path("assets/terrain/heightmap.png")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Terrain
Terrain::Terrain(glm::vec2 const& pos, DrawInfo&& di, TextureInfo const& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(ti)
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
TerrainArray::set(size_t const i, Terrain&& t)
{
  auto const c = data_.capacity();
  assert(i < c);

  data_[i] = MOVE(t);
  ++num_inserted_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainGrid
TerrainGrid::TerrainGrid(size_t const nr, size_t const nc)
    : num_rows_(nr)
    , num_cols_(nc)
{
  terrain_.reserve(num_rows_ * num_cols_);
}

TerrainGrid::TerrainGrid()
    : TerrainGrid(0, 0)
{
}

void
TerrainGrid::set(size_t const i, Terrain&& t)
{
  assert(i < terrain_.capacity());
  terrain_.set(i, MOVE(t));
}

} // namespace boomhs

namespace boomhs::terrain
{

TerrainGrid
generate(stlw::Logger& logger, TerrainConfiguration const& tc, HeightmapData const& heightmap_data,
         ShaderProgram& sp, TextureInfo const& ti)
{
  LOG_TRACE("Generating Terrain");
  size_t const rows = tc.num_rows, cols = tc.num_cols;
  TerrainGrid  tgrid{rows, cols};

  FOR(i, rows)
  {
    FOR(j, cols)
    {
      auto const pos = glm::vec2{i, j};
      auto       t   = generate_terrain_tile(logger, pos, tc, heightmap_data, sp, ti);

      auto const index = (j * rows) + i;
      tgrid.set(index, MOVE(t));
    }
  }

  LOG_TRACE("Finished Generating Terrain");
  return tgrid;
}

} // namespace boomhs::terrain
