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

int
calculate_number_indices(int const rows, int const columns)
{
  return (rows * columns) + (rows - 1) * (columns - 2);
}

ObjData::vertices_t
generate_vertices(int const rows, int const columns)
{
  int constexpr NUM_COMPONENTS     = 4; // x, y, z, w
  auto const          num_vertices = calculate_number_vertices(NUM_COMPONENTS, rows, columns);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertices);
  std::cerr << "numv: '" << num_vertices << "'\n";
  std::cerr << "rows: '" << rows << "'\n";
  std::cerr << "columns: '" << columns << "'\n";
  std::cerr << "START\n";

  static constexpr float Y = 0.0f, W = 1.0f;

  int counter = 0;
  FORI(r, rows)
  {
    std::cerr << "===========================================\n";
    FORI(c, columns)
    {
      assert(counter < num_vertices);

      int const index    = r * columns + c;
      int const offset   = NUM_COMPONENTS * index;
      buffer[offset + 0] = (float)r / (float)(rows - 1);
      ++counter;

      buffer[offset + 1] = Y;
      ++counter;

      buffer[offset + 2] = (float)c / (float)(columns - 1);
      ++counter;

      buffer[offset + 3] = W;
      ++counter;
    }
  }

  std::cerr << "vertices report:\n";
  std::cerr << "buffer capacity: '" << buffer.capacity() << "'\n";
  std::cerr << "buffer size: '" << buffer.size() << "'\n";
  std::cerr << "BUFFER CONTENTS\n";
  assert(static_cast<size_t>(counter) == buffer.size());

  int count = 0;
  FOR(i, buffer.size()) {
    int const i_mod4 = i % 4;
    if (i_mod4 == 0) {
      if (i > 0) {
        std::cerr << "\n";
      }
      std::cerr << "p" << std::to_string(count++) << ": ";
    }
    std::cerr << "'" << buffer[i] << "' ";
  }
  std::cerr << "\n================================\n";
  std::cerr << "FINISH\n";
  assert(counter == num_vertices);
  return buffer;
}

ObjData::vertices_t
generate_normals(int const rows, int const columns)
{
  auto const          num_vertices = calculate_number_vertices(3, rows, columns);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertices);

  int counter = 0;
  FORI(r, rows)
  {
    FORI(c, columns)
    {
      assert(counter < num_vertices);

      buffer[counter++] = 0.0f;
      buffer[counter++] = 1.0f;
      buffer[counter++] = 0.0f;
    }
  }
  assert(counter == num_vertices);
  return buffer;
}

ObjData::vertices_t
generate_uvs(int const rows, int const columns)
{
  auto const          num_vertices = calculate_number_vertices(2, rows, columns);
  ObjData::vertices_t buffer;
  buffer.resize(num_vertices);

  int counter = 0;
  FORI(r, rows)
  {
    FORI(c, columns)
    {
      assert(counter < num_vertices);

      float const u     = (float)c / ((float)columns - 1);
      float const v     = (float)r / ((float)rows - 1);
      buffer[counter++] = u;
      buffer[counter++] = v;
    }
  }
  assert(counter == num_vertices);
  return buffer;
}

ObjData::indices_t
generate_indices(int const rows, int const columns)
{
  int const strips_required          = rows - 1;
  int const degen_triangles_required = 2 * (strips_required - 1);
  int const vertices_perstrip        = 2 * columns;
  int const num_indices = (vertices_perstrip * strips_required) + degen_triangles_required;

  // int const num_indices = calculate_number_indices(rows, columns);
  std::cerr << "num_indices: '" << num_indices << "'\n";
  ObjData::indices_t buffer;
  buffer.resize(num_indices);

  int offset = 0;
  FORI(y, rows - 1)
  {
    if (y < 0) {
      // Degenerate begin: repeat first vertex
      buffer[offset++] = y * rows;
    }

    FORI(x, columns)
    {
      // One part of the strip
      buffer[offset++] = (y * rows) + x;
      buffer[offset++] = ((y + 1) * rows) + x;
    }

    if (y < rows - 2) {
      // Degenerate end: repeat last vertex
      buffer[offset++] = ((y + 1) * rows) + (columns - 1);
    }
  }


  std::cerr << "indices report:\n";
  std::cerr << "buffer capacity: '" << buffer.capacity() << "'\n";
  std::cerr << "buffer size: '" << buffer.size() << "'\n";
  assert(static_cast<size_t>(offset) < buffer.size());

  int triangle_count = 0;
  FOR(i, buffer.size()) {

    int const i_mod3 = i % 3;
    if (i_mod3 == 0) {
      if (i > 0) {
        std::cerr << "\n";
      }
      std::cerr << "triangle #" << std::to_string(triangle_count++) << ": ";
    }

    std::cerr << "'" << buffer[i] << "' ";
  }
  std::cerr << "\n================================\n";
  std::cerr << "FINISH\n";
  std::cerr << "offset: '" << offset << "'\n";
  assert(offset < num_indices);
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
  data.num_vertices = count;

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
int const Terrain::VERTEX_COUNT = 4;

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

Terrain
generate(stlw::Logger& logger, glm::vec2 const& pos, ShaderProgram& sp, TextureInfo const& ti)
{
  BufferFlags const flags{true, true, true, false};
  auto const        data   = generate_terrain_data(flags);
  auto const        buffer = VertexBuffer::create_interleaved(logger, data, flags);

  auto di = gpu::copy_gpu(logger, GL_TRIANGLE_STRIP, sp, buffer, ti);
  return Terrain{pos, MOVE(di), ti};
}

} // namespace boomhs::terrain
