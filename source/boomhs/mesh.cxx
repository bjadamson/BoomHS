#include <boomhs/heightmap.hpp>
#include <boomhs/mesh.hpp>
#include <cassert>
#include <common/algorithm.hpp>

using namespace boomhs;

namespace
{

int constexpr NORMAL_NUM_COMPONENTS = 3; // xn, yn, zn

// num_components => number elements per vertex
//
// ie: positions would have "3" for (x, y, z)
//
//     normals              "3" for (xn, yn, zn)
//     uvs                  "2" for (u, v)
//
// etc...
size_t
calculate_number_vertices(size_t const num_components, size_t const num_vertexes)
{
  return num_components * (num_vertexes * num_vertexes);
}

float
calculate_ratio(float const v, size_t const num_vertexes, float const length)
{
  return (v / (num_vertexes - 1)) * length;
}

ObjVertices
create_normal_buffer(size_t const num_vertexes)
{
  size_t const num_vertices = NORMAL_NUM_COMPONENTS * math::squared(num_vertexes);

  ObjVertices normals;
  normals.resize(num_vertices);
  assert(0 == (normals.size() % NORMAL_NUM_COMPONENTS));

  return normals;
}

} // namespace

namespace boomhs
{

// Algorithm modified from:
// https://www.youtube.com/watch?v=yNYwZMmgTJk&list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP&index=14
ObjVertices
MeshFactory::generate_rectangle_mesh(common::Logger& logger, glm::vec2 const& dimensions,
                                     size_t const num_vertexes)
{
  LOG_TRACE("Generating rectangle mesh begin");
  size_t constexpr NUM_COMPONENTS = 3; // x, y, z

  auto const x_length = num_vertexes, z_length = num_vertexes;
  auto const num_vertices = calculate_number_vertices(NUM_COMPONENTS, num_vertexes);

  ObjVertices buffer;
  buffer.resize(num_vertices);

  size_t offset = 0;
  assert(offset < buffer.size());
  FOR(z, z_length)
  {
    FOR(x, x_length)
    {
      assert(offset < static_cast<size_t>(num_vertices));

      float const xpos = 0.0f + (calculate_ratio(x, num_vertexes, dimensions.x));
      float const ypos = 0.0f;
      float const zpos = 0.0f + (calculate_ratio(z, num_vertexes, dimensions.y));

      assert(offset < buffer.size());
      buffer[offset++] = xpos;
      assert(offset < buffer.size());
      LOG_DEBUG_SPRINTF("xpos: %f, ypos: %f, zpos: %f", xpos, ypos, zpos);

      assert(offset < buffer.size());
      buffer[offset++] = ypos;

      assert(offset < buffer.size());
      buffer[offset++] = zpos;
    }
  }

  assert(offset == buffer.size());
  assert(offset == static_cast<size_t>(num_vertices));

  LOG_TRACE("Finished generating vertices");
  return buffer;
}

ObjVertices
MeshFactory::generate_uvs(common::Logger& logger, glm::vec2 const& dimensions,
                          size_t const num_vertexes, bool const tile)
{
  size_t constexpr NUM_COMPONENTS = 2; // u, v
  auto const num_vertices         = calculate_number_vertices(NUM_COMPONENTS, num_vertexes);
  auto const x_length = num_vertexes, z_length = num_vertexes;

  ObjVertices buffer;
  buffer.resize(num_vertices);

  size_t counter = 0;
  FOR(x, x_length)
  {
    FOR(z, z_length)
    {
      assert(counter < num_vertices);

      float u = calculate_ratio(x, num_vertexes, dimensions.x);
      float v = calculate_ratio(z, num_vertexes, dimensions.y);
      if (!tile) {
        u = u / dimensions.x;
        v = v / dimensions.y;
      }

      buffer[counter++] = u;
      buffer[counter++] = v;
    }
  }
  assert(counter == num_vertices);
  return buffer;
}

// Algorithm modified from:
// https://www.youtube.com/watch?v=yNYwZMmgTJk&list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP&index=14
ObjIndices
MeshFactory::generate_indices(common::Logger& logger, size_t const num_vertexes)
{
  auto const x_length = num_vertexes, z_length = num_vertexes;
  auto const strips_required          = z_length - 1;
  auto const degen_triangles_required = 2 * (strips_required - 1);
  auto const vertices_perstrip        = 2 * x_length;

  size_t const num_indices = (vertices_perstrip * strips_required) + degen_triangles_required;

  ObjIndices buffer;
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

ObjVertices
MeshFactory::generate_normals(common::Logger& logger, GenerateNormalData const& normal_data)
{
  auto const num_vertexes = normal_data.num_vertexes;
  auto       normals      = create_normal_buffer(num_vertexes);
  auto const width = num_vertexes, height = num_vertexes;

  //
  // Algorithm adapted from:
  // http://www.flipcode.com/archives/Calculating_Vertex_Normals_for_Height_Maps.shtml
  //
  // unsigned char h(x, y) returns the height map value at x, y.
  // the map is of size width*height
  // Vector3 normal[width*height] will contain the calculated normals.
  //
  // The height map has x, y axes with (0, 0) being the top left corner of the map.
  // The resulting mesh is assumed to be in a left hand system with x right, z into the screen
  // and y up (i.e. as in DirectX).
  //
  // yScale denotes the scale of mapping heights to final y values in model space
  // (i.e. a height difference of 1 in the height map results in a height difference
  // of yScale in the vertex coordinate).
  // xzScale denotes the same for the x, z axes. If you have different scale factors
  // for x, z then the formula becomes
  // normal[y*width+x].set(-sx*yScale, 2*xScale, xScalesy*xScale*yScale/zScale);
  float constexpr yScale  = 0.1f;
  float constexpr xzScale = yScale;
  float constexpr x0 = 0.0f, y0 = 0.0f;

  auto const& h = [&](auto const x, auto const y) { return normal_data.heightmap.data(x, y); };

  FOR(y, height)
  {
    FOR(x, width)
    {
      // The ? : and ifs are necessary for the border cases.
      float sx = h(x < width - 1 ? x + 1 : x, y) - h(x0 ? x - 1 : x, y);
      if (x == 0 || x == width - 1) {
        sx *= 2;
      }

      float sy = h(x, y < height - 1 ? y + 1 : y) - h(x, y0 ? y - 1 : y);
      if (y == 0 || y == height - 1) {
        sy *= 2;
      }

      auto const   normal = glm::normalize(glm::vec3{-sx * yScale, 2 * xzScale, sy * yScale});
      size_t const index  = NORMAL_NUM_COMPONENTS * ((y * width) + x);
      auto const   xn     = index + 0;
      auto const   yn     = index + 1;
      auto const   zn     = index + 2;
      assert(zn < normals.size());

      auto const set = [&](auto& component, float const value) {
        component = normal_data.invert_normals ? -value : value;
      };
      set(normals[xn], normal.x);
      set(normals[yn], normal.y);
      set(normals[zn], normal.z);
    }
  }
  return normals;
}

ObjVertices
MeshFactory::generate_flat_normals(common::Logger& logger, size_t const num_vertexes)
{
  auto normals = create_normal_buffer(num_vertexes);

  FOR(i, normals.size() / NORMAL_NUM_COMPONENTS)
  {
    size_t const index = NORMAL_NUM_COMPONENTS * i;
    auto const   xn    = index + 0;
    auto const   yn    = index + 1;
    auto const   zn    = index + 2;

    normals[xn] = 0.0f;
    normals[yn] = 1.0f;
    normals[zn] = 0.0f;
  }
  return normals;
}

} // namespace boomhs
