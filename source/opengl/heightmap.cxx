#include <opengl/heightmap.hpp>
#include <opengl/texture.hpp>
#include <stlw/algorithm.hpp>

using namespace boomhs;

namespace opengl::heightmap
{

ObjData::vertices_t
generate_normals(int const x_length, int const z_length, bool const invert_normals,
    HeightmapData const& heightmap_data)
{
  int constexpr NUM_COMPONENTS     = 3; // xn, yn, zn
  size_t const        num_vertices = NUM_COMPONENTS * x_length * z_length;
  ObjData::vertices_t normals;
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
  float constexpr yScale = 0.1f;
  float constexpr xzScale = yScale;
  float constexpr x0 = 0.0f, y0 = 0.0f;

  auto const width = x_length, height = z_length;
  auto const& h = [&](auto const x, auto const y)
  {
    return heightmap_data.data()[(width * y) + x];
  };
  FORI(y, height)
  {
    FORI(x, width)
    {
      // The ? : and ifs are necessary for the border cases.
      float sx = h(x<width-1 ? x+1 : x, y) - h(x0 ? x-1 : x, y);
      if (x == 0 || x == width-1)
      {
        sx *= 2;
      }

      float sy = h(x, y<height-1 ? y+1 : y) - h(x, y0 ?  y-1 : y);
      if (y == 0 || y == height -1)
      {
        sy *= 2;
      }

      auto const normal = glm::normalize(glm::vec3{-sx*yScale, 2*xzScale, sy*yScale});
      size_t const index = 3 * ((y * width) + x);
      auto const xn = index + 0;
      auto const yn = index + 1;
      auto const zn = index + 2;
      assert(zn < num_vertices);

      auto const set = [&](auto &component, float const value)
      {
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
  bool const evenly_divides_by4 = (num_pixels_in_image % 4) != 0;
  if (!evenly_divides_by4) {
    return Err(std::string{"Number of pixel fields in heightmap does not divide evenly into 4."});
  }

  HeightmapData heightmap;

  size_t const num_pixels = num_pixels_in_image;
  heightmap.reserve(num_pixels);
  auto const num_bytes = num_pixels * 4;
  for(auto i = 0u; i < num_bytes; i += 4)
  {
    auto const& data = image.data.get();
    auto const red   = data[i+0];
    auto const green = data[i+1];
    auto const blue  = data[i+2];
    auto const alpha = data[i+3];
    assert(red == green);
    assert(red == blue);
    assert(255 == alpha);
    heightmap.emplace_back(red);
  }
  assert(heightmap.size() == num_pixels);

  return Ok(heightmap);
}

HeightmapResult
parse(stlw::Logger &logger, char const* path)
{
  LOG_TRACE_SPRINTF("Loading Heightmap Data from file %s", path);
  auto const data = texture::load_image(logger, path, GL_RGBA);
  LOG_TRACE("Finished Loading Heightmap");
  return parse(data);
}

HeightmapResult
parse(stlw::Logger &logger, std::string const& path)
{
  return parse(logger, path.c_str());
}

} // ns opengl::heightmap
