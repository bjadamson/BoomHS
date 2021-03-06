#include <boomhs/heightmap.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/texture.hpp>

#include <common/algorithm.hpp>

using namespace opengl;

namespace boomhs
{

Heightmap::Heightmap(int const w)
    : width_(w)
{
}

uint8_t&
Heightmap::data(size_t const x, size_t const z)
{
  // TODO: maybe flip this around??
  return data_[(width_ * z) + x];
}

uint8_t const&
Heightmap::data(size_t const x, size_t const z) const
{
  // TODO: maybe flip this around??
  return data_[(width_ * z) + x];
}

void
Heightmap::reserve(size_t const v)
{
  data_.reserve(v);
}

void
Heightmap::add(uint8_t const v)
{
  data_.emplace_back(v);
}

std::string
Heightmap::to_string() const
{
  return fmt::sprintf("width: %i", width_);
}

} // namespace boomhs

namespace boomhs::heightmap
{

HeightmapResult
load_fromtable(common::Logger& logger, TextureTable const& ttable,
               std::string const& heightmap_path)
{
  auto const* p_heightmap = ttable.lookup_nickname(heightmap_path);
  if (!p_heightmap) {
    auto const fmt = fmt::sprintf("ERROR Looking up heightmap: %s (not found)", heightmap_path);
    LOG_ERROR(fmt);
    return Err(fmt);
  }
  auto const& hm = *p_heightmap;
  assert(1 == hm.num_filenames());
  auto const& path = hm.filenames[0];

  return heightmap::parse(logger, path);
}

ObjVertices
generate_normals(int const x_length, int const z_length, bool const invert_normals,
                 Heightmap const& heightmap)
{
  int constexpr NUM_COMPONENTS = 3; // xn, yn, zn
  size_t const num_vertices    = NUM_COMPONENTS * x_length * z_length;
  ObjVertices  normals;
  normals.resize(num_vertices);

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

  auto const  width = x_length, height = z_length;
  auto const& h = [&](auto const x, auto const y) { return heightmap.data(y, x); };
  FORI(y, height)
  {
    FORI(x, width)
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
      size_t const index  = 3 * ((y * width) + x);
      auto const   xn     = index + 0;
      auto const   yn     = index + 1;
      auto const   zn     = index + 2;
      assert(zn < num_vertices);

      auto const set = [&](auto& component, float const value) {
        component = invert_normals ? -value : value;
      };
      set(normals[xn], normal.x);
      set(normals[yn], normal.y);
      set(normals[zn], normal.z);
    }
  }
  assert(num_vertices == normals.size());
  return normals;
}

HeightmapResult
parse(ImageData const& image)
{
  auto const num_pixels_in_image = image.width * image.height;

  // The heightmap's width comes directly from the image data.
  Heightmap heightmap{image.width};

  size_t const num_pixels = num_pixels_in_image;
  heightmap.reserve(num_pixels);
  auto const num_bytes = num_pixels * 4;
  for (auto i = 0u; i < num_bytes; i += 4) {
    auto const& data  = image.data.get();
    auto const  red   = data[i + 0];
    auto const  green = data[i + 1];
    auto const  blue  = data[i + 2];
    auto const  alpha = data[i + 3];
    assert(red == green);
    assert(red == blue);
    assert(255 == alpha);
    heightmap.add(red);
  }
  assert(heightmap.size() == num_pixels);

  return Ok(MOVE(heightmap));
}

HeightmapResult
parse(common::Logger& logger, char const* path)
{
  LOG_TRACE_SPRINTF("Loading Heightmap Data from file %s", path);
  auto const data = TRY_MOVEOUT(texture::load_image(logger, path, GL_RGBA));
  LOG_TRACE("Finished Loading Heightmap");
  return parse(data);
}

HeightmapResult
parse(common::Logger& logger, std::string const& path)
{
  return parse(logger, path.c_str());
}

size_t
calculate_number_vertices(int const num_components, TerrainConfig const& tc)
{
  auto const num_vertexes = tc.num_vertexes_along_one_side;
  return num_components * (num_vertexes * num_vertexes);
}

void
update_vertices_from_heightmap(common::Logger& logger, TerrainConfig const& tc,
                               Heightmap const& heightmap, ObjVertices& buffer)
{
  LOG_TRACE("Updating vertices from heightmap");

  auto const num_vertexes_along_one_side = tc.num_vertexes_along_one_side;
  auto const x_length = num_vertexes_along_one_side, z_length = num_vertexes_along_one_side;
  size_t     offset = 0;
  FOR(z, z_length)
  {
    FOR(x, x_length)
    {
      ++offset; // skip x

      uint8_t const height = heightmap.data(x, z);
      assert(height >= 0.0f);

      float const height_normalized = height / 255.0f;
      auto const  ypos              = height_normalized * tc.height_multiplier;

      LOG_TRACE_SPRINTF("TERRAIN HEIGHT: %f (raw: %u), ypos: %f", height_normalized, height, ypos);
      buffer[offset++] = ypos;

      assert(offset < buffer.size());
      ++offset; // skip z
    }
  }

  LOG_TRACE("Finished updating vertices from heightmap");
}

} // namespace boomhs::heightmap
