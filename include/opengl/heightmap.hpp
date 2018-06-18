#pragma once
#include <opengl/texture.hpp>

#include <boomhs/obj.hpp>

#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <string>
#include <vector>

namespace opengl
{
struct ImageData;

class Heightmap
{
  using HeightmapData = std::vector<uint8_t>;
  int           width_;
  HeightmapData data_;

  COPY_DEFAULT(Heightmap);

public:
  explicit Heightmap(int);
  MOVE_DEFAULT(Heightmap);

  // methods
  Heightmap clone() const { return *this; }

  uint8_t&       data(size_t, size_t);
  uint8_t const& data(size_t, size_t) const;

  void   reserve(size_t);
  void   add(uint8_t);
  size_t size() const { return data_.size(); }

  std::string to_string() const;
};

using HeightmapResult = Result<Heightmap, std::string>;

} // namespace opengl

namespace boomhs
{
struct TerrainConfig;
} // namespace boomhs

namespace opengl::heightmap
{

HeightmapResult
load_fromtable(stlw::Logger&, TextureTable const&, std::string const&);

boomhs::ObjVertices
generate_normals(int, int, bool, Heightmap const&);

HeightmapResult
parse(ImageData const&);

HeightmapResult
parse(stlw::Logger&, char const*);

HeightmapResult
parse(stlw::Logger&, std::string const&);

void
update_vertices_from_heightmap(stlw::Logger&, boomhs::TerrainConfig const&, Heightmap const&,
                               boomhs::ObjVertices&);

} // namespace opengl::heightmap
