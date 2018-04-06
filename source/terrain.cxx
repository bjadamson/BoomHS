#include <boomhs/obj.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/buffer.hpp>
#include <opengl/gpu.hpp>
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
int
calculate_number_vertices(int const num_components, int const rows, int const columns)
{
  return num_components * columns * rows;
}

ObjData::vertices_t
generate_vertices(int const x_length, int const z_length)
{
  int constexpr NUM_COMPONENTS     = 4; // x, y, z, w
  auto const          num_vertexes = calculate_number_vertices(NUM_COMPONENTS, x_length, z_length);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertexes);

  static constexpr float Y = 0.0f, W = 1.0f;

  // For calculating ratio's inside the loop
  assert(0 != (x_length - 1));
  assert(0 != (z_length - 1));

  size_t offset = 0;
  assert(offset < buffer.size());
  FORI(z, z_length)
  {
    FORI(x, x_length)
    {
      assert(offset < static_cast<size_t>(num_vertexes));

      float const xRatio = x / (float) (x_length - 1);

      // Build our heightmap from the top down, so that our triangles are 
      // counter-clockwise.
      float const zRatio = 1.0f - (z / (float) (z_length - 1));

      static constexpr float MIN_POSITION   = 0.0f;
      static constexpr float POSITION_RANGE = 1.0f;

      float const xPosition = MIN_POSITION + (xRatio * POSITION_RANGE);
      float const zPosition = MIN_POSITION + (zRatio * POSITION_RANGE);

      assert(offset < buffer.size());
      buffer[offset++] = xPosition;

      assert(offset < buffer.size());
      buffer[offset++] = Y;

      assert(offset < buffer.size());
      buffer[offset++] = zPosition;

      assert(offset < buffer.size());
      buffer[offset++] = W;
    }
  }

  assert(offset == buffer.size());
  assert(offset == static_cast<size_t>(num_vertexes));
  return buffer;
}

ObjData::vertices_t
generate_normals(int const rows, int const columns)
{
  auto const          num_vertexes = calculate_number_vertices(3, rows, columns);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertexes);

  int counter = 0;
  FORI(r, rows)
  {
    FORI(c, columns)
    {
      assert(counter < num_vertexes);

      buffer[counter++] = 0.0f;
      buffer[counter++] = 1.0f;
      buffer[counter++] = 0.0f;
    }
  }
  assert(counter == num_vertexes);
  return buffer;
}

ObjData::vertices_t
generate_uvs(int const rows, int const columns)
{
  auto const          num_vertexes = calculate_number_vertices(2, rows, columns);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertexes);

  int counter = 0;
  FORI(r, rows)
  {
    FORI(c, columns)
    {
      assert(counter < num_vertexes);

      float const u     = (float)c / ((float)columns - 1);
      float const v     = (float)r / ((float)rows - 1);
      buffer[counter++] = u;
      buffer[counter++] = v;
    }
  }
  assert(counter == num_vertexes);
  return buffer;
}

ObjData::indices_t
generate_indices(int const x_length, int const z_length)
{
  int const strips_required          = z_length - 1;
  int const degen_triangles_required = 2 * (strips_required - 1);
  int const vertices_perstrip        = 2 * x_length;
  size_t const num_indices = (vertices_perstrip * strips_required) + degen_triangles_required;

  ObjData::indices_t buffer;
  buffer.resize(num_indices);

  size_t offset = 0;
  FORI(z, z_length - 1)
  {
    if (z > 0) {
      // Degenerate begin: repeat first vertex
      buffer[offset++] = z * z_length;
    }

    FORI(x, x_length)
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
generate_terrain_data(BufferFlags const& flags)
{
  auto const VC   = Terrain::VERTEX_COUNT;
  int const  rows = VC, columns = VC;
  int const  count = rows * columns;

  ObjData data;
  data.num_vertexes = count;

  data.vertices = generate_vertices(rows, columns);
  data.normals  = generate_normals(rows, columns);
  data.uvs      = generate_uvs(rows, columns);
  data.indices  = generate_indices(rows, columns);

  return data;
}

} // namespace

namespace boomhs
{

int const Terrain::SIZE         = 800;
int const Terrain::VERTEX_COUNT = 8;

Terrain::Terrain(glm::vec2 const& pos, DrawInfo&& di, TextureInfo const& ti)
    : pos_(pos)
    , di_(MOVE(di))
    , ti_(ti)
{
  pos_ = glm::vec2{0.0f};//pos * Terrain::SIZE;
}

} // namespace boomhs

namespace boomhs::terrain
{

Terrain generate(stlw::Logger& logger, glm::vec2 const& pos, ShaderProgram& sp, TextureInfo const& ti)
{
  LOG_TRACE("Generating Terrain");

  BufferFlags const flags{true, true, false, true};
  auto const        data   = generate_terrain_data(flags);
  LOG_DEBUG_SPRINTF("Generated terrain data: %s", data.to_string());

  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);
  auto di = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, ti);

  LOG_TRACE("Finished Generating Terrain");
  return Terrain{pos, MOVE(di), ti};
}

} // namespace boomhs::terrain
